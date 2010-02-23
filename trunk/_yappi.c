/*

 yappi.py
 Yet Another Python Profiler

 Sumer Cip 2009

*/


#include "Python.h"
#include "frameobject.h"
#include "_ycallstack.h"
#include "_yhashtab.h"
#include "_ydebug.h"
#include "_ytiming.h"
#include "_yfreelist.h"
#include "_ystatic.h"
#include "_ymem.h"

// profiler related
#define CURRENTCTX _thread2ctx(frame->f_tstate)

typedef struct {
	PyObject *co; // CodeObject or MethodDef descriptive string.
	unsigned long callcount;
	long long tsubtotal;
	long long ttotal;
}_pit;

typedef struct {
	_cstack *cs;
	long id;
	_pit *last_pit;
	unsigned long sched_cnt;
	long long ttotal;
	char *class_name;
}_ctx;

typedef struct {
	int builtins;
}_flag;


// stat related definitions
typedef struct {
	unsigned long callcount;
	double ttot;
	double tsub;
	double tavg;
	char result[LINE_LEN];
    char fname[FUNC_NAME_LEN];
}_statitem;


struct _stat_node_t{
	_statitem *it;
	struct _stat_node_t *next;
};
typedef struct _stat_node_t _statnode;

// profiler global vars
static PyObject *YappiProfileError;
static _statnode *statshead;
static _htab *contexts;
static _htab *pits;
static long long yappoverhead; 	// total profiler overhead
static _flag flags;
static _freelist *flpit;
static _freelist *flctx;
static int yappinitialized;
static int yapphavestats;	// start() called at least once or stats cleared?
static int yapprunning;
static time_t yappstarttime;
static long long yappstarttick;
static long long yappstoptick;
static _ctx *last_ctx;

static _pit *
_create_pit(void)
{
	_pit *pit;

	pit = flget(flpit);
	if (!pit)
		return NULL;
	pit->callcount = 0;
	pit->ttotal = 0;
	pit->tsubtotal = 0;
	pit->co = NULL;
	return pit;
}

static _ctx *
_create_ctx(void)
{
	_ctx *ctx = flget(flctx);
	if (!ctx)
		return NULL;
	ctx->cs = screate(100);
	if (!ctx->cs)
		return NULL;
	ctx->last_pit = NULL;
	ctx->sched_cnt = 0;
	ctx->ttotal = 0;
	ctx->id = 0;
	ctx->class_name = NULL;
	return ctx;
}

char *
_get_current_thread_class_name(void)
{
	PyObject *mthr, *cthr, *tattr1, *tattr2;

	mthr = cthr = tattr1 = tattr2 = NULL;

	mthr = PyImport_ImportModule("threading");
	if (!mthr)
		goto err;
	cthr = PyObject_CallMethod(mthr, "currentThread", "");
	if (!cthr)
		goto err;
	tattr1 = PyObject_GetAttrString(cthr, "__class__");
	if (!tattr1)
		goto err;
	tattr2 = PyObject_GetAttrString(tattr1, "__name__");
	if (!tattr2)
		goto err;

	return PyString_AS_STRING(tattr2);

err:
	Py_XDECREF(mthr);
	Py_XDECREF(cthr);
	Py_XDECREF(tattr1);
	Py_XDECREF(tattr2);
	return NULL; //continue enumeration on err.
}

static _ctx *
_thread2ctx(PyThreadState *ts)
{
	_hitem *it;

	it = hfind(contexts, (uintptr_t)ts);
	if (!it)
		return NULL;
	return (_ctx *)it->val;
}

// the pit will be cleared by the relevant freelist. we do not free it here.
// we only DECREF the CodeObject or the MethodDescriptive string.
static void
_del_pit(_pit *pit)
{
	// if it is a regular C string all DECREF will do is to decrement the first
	// character's value.
	Py_DECREF(pit->co);
}

static _pit *
_ccode2pit(void *cco)
{
	PyCFunctionObject *cfn = cco;
	_hitem *it;

	it = hfind(pits, (uintptr_t)cco);
	if (!it) {
		_pit *pit = _create_pit();
		if (!pit)
			return NULL;
		if (!hadd(pits, (uintptr_t)cco, (uintptr_t)pit))
			return NULL;
		// built-in FUNCTION?
		if (cfn->m_self == NULL) {
			PyObject *mod = cfn->m_module;
			char *modname;
			if (mod && PyString_Check(mod)) {
				modname = PyString_AS_STRING(mod);
			}
			else if (mod && PyModule_Check(mod)) {
				modname = PyModule_GetName(mod);
				if (modname == NULL) {
					PyErr_Clear();
					modname = "__builtin__";
				}
			}
			else {
				modname = "__builtin__";
			}
			if (strcmp(modname, "__builtin__") != 0)
				pit->co = PyString_FromFormat("<%s.%s>",
							   modname,
							   cfn->m_ml->ml_name);
			else
				pit->co = PyString_FromFormat("<%s>",
							   cfn->m_ml->ml_name);

		} else { // built-in METHOD?
			PyObject *self = cfn->m_self;
			PyObject *name = PyString_FromString(cfn->m_ml->ml_name);
			if (name != NULL) {
				PyObject *mo = _PyType_Lookup((PyTypeObject *)PyObject_Type(self), name);
				Py_XINCREF(mo);
				Py_DECREF(name);
				if (mo != NULL) {
					PyObject *res = PyObject_Repr(mo);
					Py_DECREF(mo);
					if (res != NULL)
						pit->co = res;
				}
			}
			PyErr_Clear();
			pit->co = PyString_FromFormat("<built-in method %s>",
						   cfn->m_ml->ml_name);
		}
 		return pit;
	}
	return ((_pit *)it->val);
}

// maps the PyCodeObject to our internal pit item via hash table.
static _pit *
_code2pit(void *co)
{
	_hitem *it;

	it = hfind(pits, (uintptr_t)co);
	if (!it) {
		_pit *pit = _create_pit();
		if (!pit)
			return NULL;
		if (!hadd(pits, (uintptr_t)co, (uintptr_t)pit))
			return NULL;
		Py_INCREF((PyObject *)co);
		pit->co = co; //dummy
 		return pit;
	}
	return ((_pit *)it->val);
}

static void
_call_enter(PyObject *self, PyFrameObject *frame, PyObject *arg)
{
	_ctx *context;
	_pit *cp;
	PyObject *last_type, *last_value, *last_tb;
	PyErr_Fetch(&last_type, &last_value, &last_tb);

	context = CURRENTCTX;

	if (!context) {
		yerr("current context not found in table.");
		return;
	}

	if (PyCFunction_Check(arg)) {
		cp = _ccode2pit((PyCFunctionObject *)arg);
	} else {
		cp = _code2pit(frame->f_code);
	}

	// something went wrong. No mem, or another error. we cannot find
	// a corresponding pit. just run away:)
	if (!cp) {
		PyErr_Restore(last_type, last_value, last_tb);
		return;
	}

	spush(context->cs, cp);    

	cp->callcount++;
	context->last_pit = cp;

	// update ctx stats
	if (last_ctx != context) {
		context->sched_cnt++;
	}
	last_ctx = context;
	if (!context->class_name)
		context->class_name = _get_current_thread_class_name();

	PyErr_Restore(last_type, last_value, last_tb);
}


static void
_call_leave(PyObject *self, PyFrameObject *frame, PyObject *arg)
{
	_pit *cp, *pp;
	_cstackitem *ci,*pi;
	_ctx *context;
	long long elapsed;

	context = CURRENTCTX;

	if (!context) {
		yerr("current context not found in table.");
		return;
	}

	ci = spop(context->cs);
	if (!ci) {
		dprintf("leaving a frame while callstack is empty.\n");
		return;
	}
	cp = ci->ckey;	pi = shead(context->cs);
	if (!pi) {
		cp->ttotal += (tickcount() - ci->t0);
		return;
	}
	pp = pi->ckey;
	elapsed = (tickcount() - ci->t0);
	if (scount(context->cs, ci->ckey) > 0) {
		cp->tsubtotal -= elapsed;
	} else {
		cp->ttotal += elapsed;
	}
	pp->tsubtotal += elapsed;

	// update ctx stats
	context->ttotal += elapsed;
	if (last_ctx != context) {
		context->sched_cnt++;
	}
	last_ctx = context;
	if (!context->class_name)
		context->class_name = _get_current_thread_class_name();

	return;
}

// context will be cleared by the free list. we do not free it here.
// we only free the context call stack.
static void
_del_ctx(_ctx * ctx)
{
	sdestroy(ctx->cs);
}

static int
_yapp_callback(PyObject *self, PyFrameObject *frame, int what,
		  PyObject *arg)
{

	long long t0 = tickcount();
	switch (what) {
		case PyTrace_CALL:
			_call_enter(self, frame, arg);
			break;
		case PyTrace_RETURN: // either normally or with an exception
			_call_leave(self, frame, arg);
			break;

#ifdef PyTrace_C_CALL	/* not defined in Python <= 2.3 */

		case PyTrace_C_CALL:
			if (PyCFunction_Check(arg) && (flags.builtins))
			    _call_enter(self, frame, arg);
			break;

		case PyTrace_C_RETURN:
		case PyTrace_C_EXCEPTION:
			if (PyCFunction_Check(arg) && (flags.builtins))
			    _call_leave(self, frame, arg);
			break;

#endif
		default:
			break;
	}
	yappoverhead += tickcount() - t0;
	return 0;
}

static void
_profile_thread(PyThreadState *ts)
{
	_ctx *ctx;
	ts->use_tracing = 1;
	ts->c_profilefunc = _yapp_callback;

	ctx = _create_ctx();
	if (!ctx)
		return;

	// If a ThreadState object is destroyed, currently(v0.2) yappi does not
	// deletes the associated resources. Instead, we rely on the fact that
	// the ThreadState objects are actually recycled. We are using pointer
	// to map to the internal contexts table, and Python VM will try to use
	// the destructed thread's pointer when a new thread is created. They are
	// pooled inside the VM. This is very hecky solution, but there is no
	// efficient and easy way to somehow know that a Python Thread is about
	// to be destructed.
	if (!hadd(contexts, (uintptr_t)ts, (uintptr_t)ctx)) {
		_del_ctx(ctx);
		if (!flput(flctx, ctx))
			yerr("Context cannot be recycled. Possible memory leak.[%d bytes]", sizeof(_ctx));
		dprintf("Context add failed. Already added?(%p, %ld)", ts,
				PyThreadState_GET()->thread_id);
	}
	ctx->id = ts->thread_id;
}

static void
_unprofile_thread(PyThreadState *ts)
{
	ts->use_tracing = 0;
	ts->c_profilefunc = NULL;
}

static void
_ensure_thread_profiled(PyThreadState *ts)
{
	PyThreadState *p = NULL;

	for (p=ts->interp->tstate_head ; p != NULL; p = p->next) {
		if (ts->c_profilefunc != _yapp_callback)
			_profile_thread(ts);
	}
}

static void
_enum_threads(void (*f) (PyThreadState *))
{
	PyThreadState *p = NULL;

	for (p=PyThreadState_GET()->interp->tstate_head ; p != NULL; p = p->next) {
		f(p);
	}
}

static int
_init_profiler(void)
{
	// already initialized? only after clear_stats() and first time, this flag
	// will be unset.
	if (!yappinitialized) {
		contexts = htcreate(HT_CTX_SIZE);
		if (!contexts)
			return 0;
		pits = htcreate(HT_PIT_SIZE);
		if (!pits)
			return 0;
		yappoverhead = 0;
		flpit = flcreate(sizeof(_pit), FL_PIT_SIZE);
		if (!flpit)
			return 0;
		flctx = flcreate(sizeof(_ctx), FL_CTX_SIZE);
		if (!flctx)
			return 0;
		yappinitialized = 1;
		statshead = NULL;
		last_ctx = NULL;
	}
	return 1;
}

static PyObject*
profile_event(PyObject *self, PyObject *args)
{
	char *ev;
	PyObject *arg;
	PyStringObject *event;
	PyFrameObject * frame;

	if (!PyArg_ParseTuple(args, "OOO", &frame, &event, &arg)) {
        	return NULL;
    }

	_ensure_thread_profiled(PyThreadState_GET());

	ev = PyString_AS_STRING(event);

	if (strcmp("call", ev)==0)
		_yapp_callback(self, frame, PyTrace_CALL, arg);
	else if (strcmp("return", ev)==0)
		_yapp_callback(self, frame, PyTrace_RETURN, arg);
	else if (strcmp("c_call", ev)==0)
		_yapp_callback(self, frame, PyTrace_C_CALL, arg);
	else if (strcmp("c_return", ev)==0)
		_yapp_callback(self, frame, PyTrace_C_RETURN, arg);
	else if (strcmp("c_exception", ev)==0)
		_yapp_callback(self, frame, PyTrace_C_EXCEPTION, arg);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject*
start(PyObject *self, PyObject *args)
{
	if (yapprunning) {
		PyErr_SetString(YappiProfileError, "profiler is already started. yappi is a per-interpreter resource.");
		return NULL;
	}

	if (!PyArg_ParseTuple(args, "i", &flags.builtins))
		return NULL;

	if (!_init_profiler()) {
		PyErr_SetString(YappiProfileError, "profiler cannot be initialized.");
		return NULL;
	}

	_enum_threads(&_profile_thread);

	yapprunning = 1;
	yapphavestats = 1;
	time (&yappstarttime);
	yappstarttick = tickcount();

	Py_INCREF(Py_None);
	return Py_None;
}

// extracts the function name from a given pit. Note that pit->co may be
// either a PyCodeObject or a descriptive string.
static char *
_item2fname(_pit *pt, int stripslashes)
{
	int i, sp;
	char *buf;
	PyObject *fname;

	if (!pt)
		return NULL;

	if (PyCode_Check(pt->co)) {
		fname =  PyString_FromFormat( "%s.%s:%d",
				PyString_AS_STRING(((PyCodeObject *)pt->co)->co_filename),
				PyString_AS_STRING(((PyCodeObject *)pt->co)->co_name),
				((PyCodeObject *)pt->co)->co_firstlineno );
	} else {
		fname = pt->co;
	}

	// get the basename
	sp = 0;
	buf = PyString_AS_STRING(fname); // TODO:memleak on buf?
	for (i=strlen(buf);i>-1;i--) {
		if ( (buf[i] == 92) || (buf[i] == 47) ) {
			sp = i+1;
			break;
		}
	}

	//DECREF should not be done on builtin funcs.
	//TODO: maybe this is problematic, too?
	//have not seen any problem, yet, in live.
	if (PyCode_Check(pt->co)) {
		Py_DECREF(fname);
	}

    //testing
    buf = &buf[sp];
    //if (strlen(buf) >= FUNC_NAME_LEN-1) {
    //   buf[FUNC_NAME_LEN-1] = '\0';
    //}
    //testing

	return buf;
}

static int
_pitenumstat(_hitem *item, void * arg)
{
	long long cumdiff;
	PyObject *efn;
	char *fname;
	_pit *pt;

	pt = (_pit *)item->val;
	if (!pt->ttotal)
		return 0;

	cumdiff = pt->ttotal - pt->tsubtotal;
	efn = (PyObject *)arg;

	fname = _item2fname(pt, 0);
	if (!fname)
		fname = "N/A";

	// We may have MT issues here!!! declaring a preenum func in yappi.py
	// does not help as we need a per-profiler sync. object for this. This means
	// additional complexity and additional overhead. Any idea on this?
	// Do we really have an mt issue here? The parameters that are sent to the
	// function does not directly use the same ones, they will copied over to the VM.
	PyObject_CallFunction(efn, "((sKff))", fname,
				pt->callcount, pt->ttotal * tickfactor(),
				cumdiff * tickfactor());

	return 0;
}

void
_ystrmovtoend(char **s)
{
	*s += strlen(*s);
}

void
_yzipstr(char *s, int size)
{
	int i, len;

	if (size <= ZIP_DOT_COUNT+ZIP_RIGHT_MARGIN_LEN)
		return;

	len = strlen(s);
	for(i=len;i<size;i++)
		s[i] = ' ';
	s[size] = '\0'; //terminate the string

	if (len > size-ZIP_DOT_COUNT-ZIP_RIGHT_MARGIN_LEN) {
		for(i=1;i<=ZIP_DOT_COUNT ;i++)
			s[size-i-ZIP_RIGHT_MARGIN_LEN] = '.';
		for(i=1;i<=ZIP_RIGHT_MARGIN_LEN ;i++)
			s[size-i] = ' ';
	}
}

void
_yformat_string(char *a, char *s, int size)
{
    int slen;

    slen = strlen(a);
    if (slen >= size-1) {
       a[size-1] = '\0';
    }

	_ystrmovtoend(&s);
	sprintf(s, "%s", a);
	_yzipstr(s, size);
}

void
_yformat_double(double a, char *s, int size)
{
	_ystrmovtoend(&s);
	sprintf(s, "%0.6f", a);
	_yzipstr(s, size);
}

void
_yformat_ulong(unsigned long a, char *s, int size)
{
	_ystrmovtoend(&s);
	sprintf(s, "%lu", a);
	_yzipstr(s, size);
}

void
_yformat_long(long a, char *s, int size)
{
	_ystrmovtoend(&s);
	sprintf(s, "%ld", a);
	_yzipstr(s, size);
}

void
_yformat_int(int a, char *s, int size)
{
	_ystrmovtoend(&s);
	sprintf(s, "%d", a);
	_yzipstr(s, size);
}

_statitem *
_create_statitem(char *fname, unsigned long callcount, double ttot, double tsub, double tavg)
{
	_statitem *si;

	si = (_statitem *)ymalloc(sizeof(_statitem));
	if (!si)
		return NULL;

	// init the stat item fields.
	memset(si->fname, 0, FUNC_NAME_LEN);
	memset(si->result, 0, LINE_LEN);

    _yformat_string(fname, si->fname, FUNC_NAME_LEN);
	si->callcount = callcount;
	si->ttot = ttot;
	si->tsub = tsub;
	si->tavg = tavg;

	// generate the result string field.
	_yformat_string(fname, si->result, FUNC_NAME_LEN);
    _yformat_ulong(callcount, si->result, INT_COLUMN_LEN);
	_yformat_double(ttot, si->result, DOUBLE_COLUMN_LEN);
	_yformat_double(tsub, si->result, DOUBLE_COLUMN_LEN);
	_yformat_double(tavg, si->result, DOUBLE_COLUMN_LEN);


	return si;
}

// inserts items to statshead pointed linked list for later usage according to the
// sorttype param. Note that sorting is descending by default. Read reverse from list
// to have a ascending order.
void
_insert_stats_internal(_statnode *sn, int sorttype)
{
	_statnode *p, *prev;

	prev = NULL;
	p = statshead;
	while(p) {
		//dprintf("sn:%p, sn->it:%p : p:%p, p->it:%p.\n", sn, sn->it, p, p->it);
		if (sorttype == STAT_SORT_TIME_TOTAL) {
			if (sn->it->ttot > p->it->ttot)
				break;
		} else if (sorttype == STAT_SORT_CALL_COUNT) {
			if (sn->it->callcount > p->it->callcount)
				break;
		} else if (sorttype == STAT_SORT_TIME_SUB) {
			if (sn->it->tsub > p->it->tsub)
				break;
		} else if (sorttype == STAT_SORT_TIME_AVG) {
			if (sn->it->tavg > p->it->tavg)
				break;
		} else if (sorttype == STAT_SORT_FUNC_NAME) {
			if (strcmp(sn->it->fname, p->it->fname) > 0)
				break;
		}
		prev = p;
		p = p->next;
	}

	// insert at head
	if (!prev) {
		sn->next = statshead;
		statshead = sn;
	} else {
		sn->next = prev->next;
		prev->next = sn;
	}
}

// reverses the statshead list according to a given order.
void
_order_stats_internal(int order)
{
	_statnode *p,*tmp,*pr;

	if (order == STAT_SORT_DESCENDING) {
		; // nothing to do as internal order is by default descending
	} else if (order == STAT_SORT_ASCENDING) {
		   // reverse stat linked list
		   pr = tmp = NULL;
		   p = statshead;
		   while (p != NULL) {
			  tmp  = p->next;
		      p->next = pr;
		      pr = p;
		      p = tmp;
		   }
		   statshead = pr;
	}
}

void
_clear_stats_internal(void)
{
	_statnode *p,*next;

	p = statshead;
	while(p){
		next = p->next;
		yfree(p->it);
		yfree(p);
		p = next;
	}
	statshead = NULL;
}

static int
_pitenumstat2(_hitem *item, void * arg)
{
	_pit *pt;
	char *fname;
	_statitem *si;
	long long cumdiff;
	_statnode *sni;

	pt = (_pit *)item->val;
	if (!pt->ttotal)
		return 0;

	cumdiff = pt->ttotal - pt->tsubtotal;
	fname = _item2fname(pt, 1);
	if (!fname)
		fname = "N/A";

	si = _create_statitem(fname, pt->callcount, pt->ttotal * tickfactor(),
			cumdiff * tickfactor(), pt->ttotal * tickfactor() / pt->callcount);
	if (!si)
		return 1; // abort enumeration
	sni = (_statnode *)ymalloc(sizeof(_statnode));
	if (!sni)
		return 1; // abort enumeration
	sni->it = si;

	_insert_stats_internal(sni, (int)arg);

	return 0;
}

static int
_pitenumdel(_hitem *item, void *arg)
{
	_del_pit((_pit *)item->val);
	return 0;
}

static int
_ctxenumdel(_hitem *item, void *arg)
{
	_del_ctx(((_ctx *)item->val) );
	return 0;
}

static int
_ctxenumstat(_hitem *item, void *arg)
{
	char *fname, *tcname;
	_ctx * ctx;
	char temp[LINE_LEN];
	PyObject *buf;

	ctx = (_ctx *)item->val;

	fname = _item2fname(ctx->last_pit, 1);

	if (!fname)
		fname = "N/A";

	memset(temp, 0, LINE_LEN);

	tcname = ctx->class_name;
	if (tcname == NULL)
		tcname = "N/A";

	_yformat_string(tcname, temp, THREAD_NAME_LEN);
	_yformat_long(ctx->id, temp, TID_COLUMN_LEN);
	_yformat_string(fname, temp, FUNC_NAME_LEN);
	_yformat_ulong(ctx->sched_cnt, temp, INT_COLUMN_LEN);
	_yformat_double(ctx->ttotal * tickfactor(), temp, DOUBLE_COLUMN_LEN);


	buf = PyString_FromString(temp);
	if (!buf)
		return 0; // just continue.

	if (PyList_Append((PyObject *)arg, buf) < 0)
		return 0; // just continue.

	return 0;
}

static PyObject*
stop(PyObject *self, PyObject *args)
{
	if (!yapprunning) {
		PyErr_SetString(YappiProfileError, "profiler is not started yet.");
		return NULL;
	}

	_enum_threads(&_unprofile_thread);

	yapprunning = 0;
	yappstoptick = tickcount();

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject*
clear_stats(PyObject *self, PyObject *args)
{
	if (yapprunning) {
		PyErr_SetString(YappiProfileError,
			"profiler is running. Stop profiler before clearing stats.");
		return NULL;
	}

	henum(pits, _pitenumdel, NULL);
	htdestroy(pits);
	henum(contexts, _ctxenumdel, NULL);
	htdestroy(contexts);

	fldestroy(flpit);
	fldestroy(flctx);
	yappinitialized = 0;
	yapphavestats = 0;

	YMEMLEAKCHECK();

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject*
get_stats(PyObject *self, PyObject *args)
{
	char *prof_state;
	_statnode *p;
	PyObject *buf,*li;
	int type, order, limit, fcnt;
	char temp[LINE_LEN];
	long long appttotal;

	li = buf = NULL;

	if (!yapphavestats) {
		PyErr_SetString(YappiProfileError, "profiler do not have any statistics. not started?");
		goto err;
	}

	if (!PyArg_ParseTuple(args, "iii", &type, &order, &limit)) {
		PyErr_SetString(YappiProfileError, "invalid param to get_stats");
		goto err;
	}
	// sorttype/order/limit is in valid bounds?
	if ((type < 0) || (type > STAT_SORT_TYPE_MAX)) {
		PyErr_SetString(YappiProfileError, "sorttype param for get_stats is out of bounds");
		goto err;
	}
	if ((order < 0) || (order > STAT_SORT_ORDER_MAX)) {
		PyErr_SetString(YappiProfileError, "sortorder param for get_stats is out of bounds");
		goto err;
	}
	if (limit < STAT_SHOW_ALL) {
		PyErr_SetString(YappiProfileError, "limit param for get_stats is out of bounds");
		goto err;
	}



	// enum and present stats in a linked list.(statshead)
	henum(pits, _pitenumstat2, (void *)type);
	_order_stats_internal(order);

	li = PyList_New(0);
	if (!li)
		goto err;
	if (PyList_Append(li, PyString_FromString(STAT_HEADER_STR)) < 0)
		goto err;

	fcnt = 0;
	p = statshead;
	while(p){
		// limit reached?
		if (limit != STAT_SHOW_ALL) {
			if (fcnt == limit)
				break;
		}
		buf = PyString_FromString(p->it->result);
		if (!buf)
			goto err;
		if (PyList_Append(li, buf) < 0)
			goto err;

		Py_DECREF(buf);
		fcnt++;
		p = p->next;
	}

	if (PyList_Append(li, PyString_FromString(STAT_FOOTER_STR)) < 0)
		goto err;

	henum(contexts, _ctxenumstat, (void *)li);

	if (PyList_Append(li, PyString_FromString(STAT_FOOTER_STR2)) < 0)
			goto err;

	if (yapprunning) {
		appttotal = tickcount()-yappstarttick;
		prof_state = "running";
	} else {
		appttotal = yappstoptick - yappstarttick;
		prof_state = "stopped";
	}

	memset(temp, 0, LINE_LEN);
	_yformat_string(prof_state, temp, DOUBLE_COLUMN_LEN);
	_yformat_string(ctime(&yappstarttime), temp, TIMESTR_COLUMN_LEN);
	_yformat_int(hcount(pits), temp, INT_COLUMN_LEN);
	_yformat_int(hcount(contexts), temp, INT_COLUMN_LEN);
	_yformat_ulong(ymemusage(), temp, INT_COLUMN_LEN);

	if (PyList_Append(li, PyString_FromString(temp)) < 0)
			goto err;

	if (snprintf(temp, LINE_LEN, "\n\n	yappi overhead: %0.6f/%0.6f(%%%0.6f)\n",
			yappoverhead * tickfactor(), appttotal * tickfactor(),
			((yappoverhead * tickfactor() )) / (appttotal) * 100) == -1) {
		PyErr_SetString(YappiProfileError, "output string cannot be formatted correctly. Stack corrupted?");
		goto err;
	}
	if (PyList_Append(li, PyString_FromString(temp)) < 0)
		goto err;

	// clear the internal pit stat items that are generated temporarily.
	_clear_stats_internal();

	return li;
err:
	_clear_stats_internal();
	Py_XDECREF(li);
	Py_XDECREF(buf);
	return NULL;
}

static PyObject*
enum_stats(PyObject *self, PyObject *args)
{
	PyObject *enumfn;

	if (!yapphavestats) {
		PyErr_SetString(YappiProfileError, "profiler do not have any statistics. not started?");
		return NULL;
	}

	if (!PyArg_ParseTuple(args, "O", &enumfn)) {
		PyErr_SetString(YappiProfileError, "invalid param to enum_stats");
		return NULL;
	}

	if (!PyCallable_Check(enumfn)) {
	   	PyErr_SetString(YappiProfileError, "enum function must be callable");
	   	return NULL;
    }

	henum(pits, _pitenumstat, enumfn);
	//_print_footer();

	Py_INCREF(Py_None);
	return Py_None;
}

static PyMethodDef yappi_methods[] = {
	{"start", start, METH_VARARGS, NULL},
	{"stop", stop, METH_VARARGS, NULL},
	{"get_stats", get_stats, METH_VARARGS, NULL},
	{"enum_stats", enum_stats, METH_VARARGS, NULL},
	{"clear_stats", clear_stats, METH_VARARGS, NULL},
	{"profile_event", profile_event, METH_VARARGS, NULL}, // for internal usage. do not call this.
	{NULL, NULL}      /* sentinel */
};


PyMODINIT_FUNC
init_yappi(void)
{
	PyObject *m, *d;

	m = Py_InitModule("_yappi",  yappi_methods);
	if (m == NULL)
		return;
	d = PyModule_GetDict(m);
	YappiProfileError = PyErr_NewException("_yappi.error", NULL, NULL);
	PyDict_SetItemString(d, "error", YappiProfileError);

	// add int constants
	PyModule_AddIntConstant(m, "SORTTYPE_NAME", STAT_SORT_FUNC_NAME);
	PyModule_AddIntConstant(m, "SORTTYPE_NCALL", STAT_SORT_CALL_COUNT);
	PyModule_AddIntConstant(m, "SORTTYPE_TTOTAL", STAT_SORT_TIME_TOTAL);
	PyModule_AddIntConstant(m, "SORTTYPE_TSUB", STAT_SORT_TIME_SUB);
	PyModule_AddIntConstant(m, "SORTTYPE_TAVG", STAT_SORT_TIME_AVG);
	PyModule_AddIntConstant(m, "SORTORDER_ASCENDING", STAT_SORT_ASCENDING);
	PyModule_AddIntConstant(m, "SORTORDER_DESCENDING", STAT_SORT_DESCENDING);
	PyModule_AddIntConstant(m, "SHOW_ALL", STAT_SHOW_ALL);

	// init the profiler memory and internal constants
	yappinitialized = 0;
	yapphavestats = 0;
	yapprunning = 0;

	if (!_init_profiler()) {
		PyErr_SetString(YappiProfileError, "profiler cannot be initialized.");
		return;
	}
}
