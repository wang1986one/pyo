/*************************************************************************
 * Copyright 2010 Olivier Belanger                                        *                  
 *                                                                        * 
 * This file is part of pyo, a python module to help digital signal       *
 * processing script creation.                                            *  
 *                                                                        * 
 * pyo is free software: you can redistribute it and/or modify            *
 * it under the terms of the GNU General Public License as published by   *
 * the Free Software Foundation, either version 3 of the License, or      *
 * (at your option) any later version.                                    * 
 *                                                                        *
 * pyo is distributed in the hope that it will be useful,                 *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of         *    
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 * GNU General Public License for more details.                           *
 *                                                                        *
 * You should have received a copy of the GNU General Public License      *
 * along with pyo.  If not, see <http://www.gnu.org/licenses/>.           *
 *************************************************************************/

#include <Python.h>
#include "structmember.h"
#include "pyomodule.h"
#include "streammodule.h"
#include "servermodule.h"
#include "dummymodule.h"
#include "tablemodule.h"

typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *min;
    PyObject *max;
    Stream *min_stream;
    Stream *max_stream;
    float value;
    float currentValue;
    float time;
    int timeStep;
    float stepVal;
    int timeCount;
    int modebuffer[4]; // need at least 2 slots for mul & add 
} TrigRand;

static void
TrigRand_generate_ii(TrigRand *self) {
    int i;
    float *in = Stream_getData((Stream *)self->input_stream);
    float mi = PyFloat_AS_DOUBLE(self->min);
    float ma = PyFloat_AS_DOUBLE(self->max);
    float range = ma - mi;
    
    for (i=0; i<self->bufsize; i++) {
        if (in[i] == 1) {
            self->timeCount = 0;
            self->value = range * (rand()/((float)(RAND_MAX)+1)) + mi;
            if (self->time <= 0.0)
                self->currentValue = self->value;
            else
                self->stepVal = (self->value - self->currentValue) / self->timeStep;
        }
        
        if (self->timeCount == (self->timeStep - 1)) {
            self->currentValue = self->value;
            self->timeCount++;
        }
        else if (self->timeCount < self->timeStep) {
            self->currentValue += self->stepVal;
            self->timeCount++;
        }
        
        self->data[i] = self->currentValue;
    }
}

static void
TrigRand_generate_ai(TrigRand *self) {
    int i;
    float *in = Stream_getData((Stream *)self->input_stream);
    float *mi = Stream_getData((Stream *)self->min_stream);
    float ma = PyFloat_AS_DOUBLE(self->max);
    
    for (i=0; i<self->bufsize; i++) {
        float range = ma - mi[i];
        if (in[i] == 1) {
            self->timeCount = 0;
            self->value = range * (rand()/((float)(RAND_MAX)+1)) + mi[i];
            if (self->time <= 0.0)
                self->currentValue = self->value;
            else
                self->stepVal = (self->value - self->currentValue) / self->timeStep;
        }

        if (self->timeCount == (self->timeStep - 1)) {
            self->currentValue = self->value;
            self->timeCount++;
        }
        else if (self->timeCount < self->timeStep) {
            self->currentValue += self->stepVal;
            self->timeCount++;
        }
        
        self->data[i] = self->currentValue;        
    }
}

static void
TrigRand_generate_ia(TrigRand *self) {
    int i;
    float *in = Stream_getData((Stream *)self->input_stream);
    float mi = PyFloat_AS_DOUBLE(self->min);
    float *ma = Stream_getData((Stream *)self->max_stream);
    
    for (i=0; i<self->bufsize; i++) {
        float range = ma[i] - mi;
        if (in[i] == 1) {
            self->timeCount = 0;
            self->value = range * (rand()/((float)(RAND_MAX)+1)) + mi;
            if (self->time <= 0.0)
                self->currentValue = self->value;
            else
                self->stepVal = (self->value - self->currentValue) / self->timeStep;
        }
        
        if (self->timeCount == (self->timeStep - 1)) {
            self->currentValue = self->value;
            self->timeCount++;
        }
        else if (self->timeCount < self->timeStep) {
            self->currentValue += self->stepVal;
            self->timeCount++;
        }
        
        self->data[i] = self->currentValue;
    }
}

static void
TrigRand_generate_aa(TrigRand *self) {
    int i;
    float *in = Stream_getData((Stream *)self->input_stream);
    float *mi = Stream_getData((Stream *)self->min_stream);
    float *ma = Stream_getData((Stream *)self->max_stream);
    
    for (i=0; i<self->bufsize; i++) {
        float range = ma[i] - mi[i];
        if (in[i] == 1) {
            self->timeCount = 0;
            self->value = range * (rand()/((float)(RAND_MAX)+1)) + mi[i];
            if (self->time <= 0.0)
                self->currentValue = self->value;
            else
                self->stepVal = (self->value - self->currentValue) / self->timeStep;
        }
        
        if (self->timeCount == (self->timeStep - 1)) {
            self->currentValue = self->value;
            self->timeCount++;
        }
        else if (self->timeCount < self->timeStep) {
            self->currentValue += self->stepVal;
            self->timeCount++;
        }
        
        self->data[i] = self->currentValue;
    }
}

static void TrigRand_postprocessing_ii(TrigRand *self) { POST_PROCESSING_II };
static void TrigRand_postprocessing_ai(TrigRand *self) { POST_PROCESSING_AI };
static void TrigRand_postprocessing_ia(TrigRand *self) { POST_PROCESSING_IA };
static void TrigRand_postprocessing_aa(TrigRand *self) { POST_PROCESSING_AA };
static void TrigRand_postprocessing_ireva(TrigRand *self) { POST_PROCESSING_IREVA };
static void TrigRand_postprocessing_areva(TrigRand *self) { POST_PROCESSING_AREVA };
static void TrigRand_postprocessing_revai(TrigRand *self) { POST_PROCESSING_REVAI };
static void TrigRand_postprocessing_revaa(TrigRand *self) { POST_PROCESSING_REVAA };
static void TrigRand_postprocessing_revareva(TrigRand *self) { POST_PROCESSING_REVAREVA };

static void
TrigRand_setProcMode(TrigRand *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2] + self->modebuffer[3] * 10;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;
    
	switch (procmode) {
        case 0:    
            self->proc_func_ptr = TrigRand_generate_ii;
            break;
        case 1:    
            self->proc_func_ptr = TrigRand_generate_ai;
            break;
        case 10:    
            self->proc_func_ptr = TrigRand_generate_ia;
            break;
        case 11:    
            self->proc_func_ptr = TrigRand_generate_aa;
            break;
    } 
	switch (muladdmode) {
        case 0:        
            self->muladd_func_ptr = TrigRand_postprocessing_ii;
            break;
        case 1:    
            self->muladd_func_ptr = TrigRand_postprocessing_ai;
            break;
        case 2:    
            self->muladd_func_ptr = TrigRand_postprocessing_revai;
            break;
        case 10:        
            self->muladd_func_ptr = TrigRand_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = TrigRand_postprocessing_aa;
            break;
        case 12:    
            self->muladd_func_ptr = TrigRand_postprocessing_revaa;
            break;
        case 20:        
            self->muladd_func_ptr = TrigRand_postprocessing_ireva;
            break;
        case 21:    
            self->muladd_func_ptr = TrigRand_postprocessing_areva;
            break;
        case 22:    
            self->muladd_func_ptr = TrigRand_postprocessing_revareva;
            break;
    }  
}

static void
TrigRand_compute_next_data_frame(TrigRand *self)
{
    (*self->proc_func_ptr)(self); 
    (*self->muladd_func_ptr)(self);
    Stream_setData(self->stream, self->data);
}

static int
TrigRand_traverse(TrigRand *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    Py_VISIT(self->min);    
    Py_VISIT(self->min_stream);    
    Py_VISIT(self->max);    
    Py_VISIT(self->max_stream);    
    return 0;
}

static int 
TrigRand_clear(TrigRand *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    Py_CLEAR(self->min);    
    Py_CLEAR(self->min_stream);    
    Py_CLEAR(self->max);    
    Py_CLEAR(self->max_stream);    
    return 0;
}

static void
TrigRand_dealloc(TrigRand* self)
{
    free(self->data);
    TrigRand_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * TrigRand_deleteStream(TrigRand *self) { DELETE_STREAM };

static PyObject *
TrigRand_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    TrigRand *self;
    self = (TrigRand *)type->tp_alloc(type, 0);
    
    self->min = PyFloat_FromDouble(0.);
    self->max = PyFloat_FromDouble(1.);
    self->value = self->currentValue = 0.;
    self->time = 0.0;
    self->timeCount = 0;
    self->stepVal = 0.0;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;
	self->modebuffer[3] = 0;
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, TrigRand_compute_next_data_frame);
    self->mode_func_ptr = TrigRand_setProcMode;
    return (PyObject *)self;
}

static int
TrigRand_init(TrigRand *self, PyObject *args, PyObject *kwds)
{
    float inittmp = 0.0;
    PyObject *inputtmp, *input_streamtmp, *mintmp=NULL, *maxtmp=NULL, *multmp=NULL, *addtmp=NULL;
    
    static char *kwlist[] = {"input", "min", "max", "port", "init", "mul", "add", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|OOffOO", kwlist, &inputtmp, &mintmp, &maxtmp, &self->time, &inittmp, &multmp, &addtmp))
        return -1; 
    
    Py_XDECREF(self->input);
    self->input = inputtmp;
    input_streamtmp = PyObject_CallMethod((PyObject *)self->input, "_getStream", NULL);
    Py_INCREF(input_streamtmp);
    Py_XDECREF(self->input_stream);
    self->input_stream = (Stream *)input_streamtmp;
    
    if (mintmp) {
        PyObject_CallMethod((PyObject *)self, "setMin", "O", mintmp);
    }
    
    if (maxtmp) {
        PyObject_CallMethod((PyObject *)self, "setMax", "O", maxtmp);
    }
    
    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }
    
    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }
    
    Py_INCREF(self->stream);
    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    srand((unsigned)(time(0)));
    self->value = self->currentValue = inittmp;
    self->timeStep = (int)(self->time * self->sr);

    (*self->mode_func_ptr)(self);
    
    TrigRand_compute_next_data_frame((TrigRand *)self);
    
    Py_INCREF(self);
    return 0;
}

static PyObject * TrigRand_getServer(TrigRand* self) { GET_SERVER };
static PyObject * TrigRand_getStream(TrigRand* self) { GET_STREAM };
static PyObject * TrigRand_setMul(TrigRand *self, PyObject *arg) { SET_MUL };	
static PyObject * TrigRand_setAdd(TrigRand *self, PyObject *arg) { SET_ADD };	
static PyObject * TrigRand_setSub(TrigRand *self, PyObject *arg) { SET_SUB };	
static PyObject * TrigRand_setDiv(TrigRand *self, PyObject *arg) { SET_DIV };	

static PyObject * TrigRand_play(TrigRand *self) { PLAY };
static PyObject * TrigRand_out(TrigRand *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * TrigRand_stop(TrigRand *self) { STOP };

static PyObject * TrigRand_multiply(TrigRand *self, PyObject *arg) { MULTIPLY };
static PyObject * TrigRand_inplace_multiply(TrigRand *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * TrigRand_add(TrigRand *self, PyObject *arg) { ADD };
static PyObject * TrigRand_inplace_add(TrigRand *self, PyObject *arg) { INPLACE_ADD };
static PyObject * TrigRand_sub(TrigRand *self, PyObject *arg) { SUB };
static PyObject * TrigRand_inplace_sub(TrigRand *self, PyObject *arg) { INPLACE_SUB };
static PyObject * TrigRand_div(TrigRand *self, PyObject *arg) { DIV };
static PyObject * TrigRand_inplace_div(TrigRand *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
TrigRand_setMin(TrigRand *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);
	
	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->min);
	if (isNumber == 1) {
		self->min = PyNumber_Float(tmp);
        self->modebuffer[2] = 0;
	}
	else {
		self->min = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->min, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->min_stream);
        self->min_stream = (Stream *)streamtmp;
		self->modebuffer[2] = 1;
	}
    
    (*self->mode_func_ptr)(self);
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyObject *
TrigRand_setMax(TrigRand *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);
	
	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->max);
	if (isNumber == 1) {
		self->max = PyNumber_Float(tmp);
        self->modebuffer[3] = 0;
	}
	else {
		self->max = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->max, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->max_stream);
        self->max_stream = (Stream *)streamtmp;
		self->modebuffer[3] = 1;
	}
    
    (*self->mode_func_ptr)(self);
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyObject *
TrigRand_setPort(TrigRand *self, PyObject *arg)
{
	PyObject *tmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);
	
	tmp = arg;
	Py_INCREF(tmp);
	if (isNumber == 1) {
		self->time = PyFloat_AS_DOUBLE(PyNumber_Float(tmp));
        self->timeStep = (int)(self->time * self->sr);
	}
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyMemberDef TrigRand_members[] = {
{"server", T_OBJECT_EX, offsetof(TrigRand, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(TrigRand, stream), 0, "Stream object."},
{"input", T_OBJECT_EX, offsetof(TrigRand, input), 0, "Input sound object."},
{"min", T_OBJECT_EX, offsetof(TrigRand, min), 0, "Minimum possible value."},
{"max", T_OBJECT_EX, offsetof(TrigRand, max), 0, "Maximum possible value."},
{"mul", T_OBJECT_EX, offsetof(TrigRand, mul), 0, "Mul factor."},
{"add", T_OBJECT_EX, offsetof(TrigRand, add), 0, "Add factor."},
{NULL}  /* Sentinel */
};

static PyMethodDef TrigRand_methods[] = {
{"getServer", (PyCFunction)TrigRand_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)TrigRand_getStream, METH_NOARGS, "Returns stream object."},
{"deleteStream", (PyCFunction)TrigRand_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
{"play", (PyCFunction)TrigRand_play, METH_NOARGS, "Starts computing without sending sound to soundcard."},
{"out", (PyCFunction)TrigRand_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
{"stop", (PyCFunction)TrigRand_stop, METH_NOARGS, "Stops computing."},
{"setMin", (PyCFunction)TrigRand_setMin, METH_O, "Sets minimum possible value."},
{"setMax", (PyCFunction)TrigRand_setMax, METH_O, "Sets maximum possible value."},
{"setPort", (PyCFunction)TrigRand_setPort, METH_O, "Sets a new ramp time value."},
{"setMul", (PyCFunction)TrigRand_setMul, METH_O, "Sets oscillator mul factor."},
{"setAdd", (PyCFunction)TrigRand_setAdd, METH_O, "Sets oscillator add factor."},
{"setSub", (PyCFunction)TrigRand_setSub, METH_O, "Sets inverse add factor."},
{"setDiv", (PyCFunction)TrigRand_setDiv, METH_O, "Sets inverse mul factor."},
{NULL}  /* Sentinel */
};

static PyNumberMethods TrigRand_as_number = {
(binaryfunc)TrigRand_add,                         /*nb_add*/
(binaryfunc)TrigRand_sub,                         /*nb_subtract*/
(binaryfunc)TrigRand_multiply,                    /*nb_multiply*/
(binaryfunc)TrigRand_div,                                              /*nb_divide*/
0,                                              /*nb_remainder*/
0,                                              /*nb_divmod*/
0,                                              /*nb_power*/
0,                                              /*nb_neg*/
0,                                              /*nb_pos*/
0,                                              /*(unaryfunc)array_abs,*/
0,                                              /*nb_nonzero*/
0,                                              /*nb_invert*/
0,                                              /*nb_lshift*/
0,                                              /*nb_rshift*/
0,                                              /*nb_and*/
0,                                              /*nb_xor*/
0,                                              /*nb_or*/
0,                                              /*nb_coerce*/
0,                                              /*nb_int*/
0,                                              /*nb_long*/
0,                                              /*nb_float*/
0,                                              /*nb_oct*/
0,                                              /*nb_hex*/
(binaryfunc)TrigRand_inplace_add,                 /*inplace_add*/
(binaryfunc)TrigRand_inplace_sub,                 /*inplace_subtract*/
(binaryfunc)TrigRand_inplace_multiply,            /*inplace_multiply*/
(binaryfunc)TrigRand_inplace_div,                                              /*inplace_divide*/
0,                                              /*inplace_remainder*/
0,                                              /*inplace_power*/
0,                                              /*inplace_lshift*/
0,                                              /*inplace_rshift*/
0,                                              /*inplace_and*/
0,                                              /*inplace_xor*/
0,                                              /*inplace_or*/
0,                                              /*nb_floor_divide*/
0,                                              /*nb_true_divide*/
0,                                              /*nb_inplace_floor_divide*/
0,                                              /*nb_inplace_true_divide*/
0,                                              /* nb_index */
};

PyTypeObject TrigRandType = {
PyObject_HEAD_INIT(NULL)
0,                                              /*ob_size*/
"_pyo.TrigRand_base",                                   /*tp_name*/
sizeof(TrigRand),                                 /*tp_basicsize*/
0,                                              /*tp_itemsize*/
(destructor)TrigRand_dealloc,                     /*tp_dealloc*/
0,                                              /*tp_print*/
0,                                              /*tp_getattr*/
0,                                              /*tp_setattr*/
0,                                              /*tp_compare*/
0,                                              /*tp_repr*/
&TrigRand_as_number,                              /*tp_as_number*/
0,                                              /*tp_as_sequence*/
0,                                              /*tp_as_mapping*/
0,                                              /*tp_hash */
0,                                              /*tp_call*/
0,                                              /*tp_str*/
0,                                              /*tp_getattro*/
0,                                              /*tp_setattro*/
0,                                              /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
"TrigRand objects. Generates a new random value on a trigger signal.",           /* tp_doc */
(traverseproc)TrigRand_traverse,                  /* tp_traverse */
(inquiry)TrigRand_clear,                          /* tp_clear */
0,                                              /* tp_richcompare */
0,                                              /* tp_weaklistoffset */
0,                                              /* tp_iter */
0,                                              /* tp_iternext */
TrigRand_methods,                                 /* tp_methods */
TrigRand_members,                                 /* tp_members */
0,                                              /* tp_getset */
0,                                              /* tp_base */
0,                                              /* tp_dict */
0,                                              /* tp_descr_get */
0,                                              /* tp_descr_set */
0,                                              /* tp_dictoffset */
(initproc)TrigRand_init,                          /* tp_init */
0,                                              /* tp_alloc */
TrigRand_new,                                     /* tp_new */
};

/*********************************************************************************************/
/* TrigChoice ********************************************************************************/
/*********************************************************************************************/
typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    int chSize;
    float *choice;
    float value;
    float currentValue;
    float time;
    int timeStep;
    float stepVal;
    int timeCount;
    int modebuffer[2]; // need at least 2 slots for mul & add 
} TrigChoice;

static void
TrigChoice_generate(TrigChoice *self) {
    int i;
    float *in = Stream_getData((Stream *)self->input_stream);
    
    for (i=0; i<self->bufsize; i++) {
        if (in[i] == 1) {
            self->timeCount = 0;
            self->value = self->choice[(int)((rand()/((float)(RAND_MAX))) * self->chSize)];
            if (self->time <= 0.0)
                self->currentValue = self->value;
            else
                self->stepVal = (self->value - self->currentValue) / self->timeStep;
        }
        
        if (self->timeCount == (self->timeStep - 1)) {
            self->currentValue = self->value;
            self->timeCount++;
        }
        else if (self->timeCount < self->timeStep) {
            self->currentValue += self->stepVal;
            self->timeCount++;
        }
        
        self->data[i] = self->currentValue;
    }
}

static void TrigChoice_postprocessing_ii(TrigChoice *self) { POST_PROCESSING_II };
static void TrigChoice_postprocessing_ai(TrigChoice *self) { POST_PROCESSING_AI };
static void TrigChoice_postprocessing_ia(TrigChoice *self) { POST_PROCESSING_IA };
static void TrigChoice_postprocessing_aa(TrigChoice *self) { POST_PROCESSING_AA };
static void TrigChoice_postprocessing_ireva(TrigChoice *self) { POST_PROCESSING_IREVA };
static void TrigChoice_postprocessing_areva(TrigChoice *self) { POST_PROCESSING_AREVA };
static void TrigChoice_postprocessing_revai(TrigChoice *self) { POST_PROCESSING_REVAI };
static void TrigChoice_postprocessing_revaa(TrigChoice *self) { POST_PROCESSING_REVAA };
static void TrigChoice_postprocessing_revareva(TrigChoice *self) { POST_PROCESSING_REVAREVA };

static void
TrigChoice_setProcMode(TrigChoice *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;
    
    self->proc_func_ptr = TrigChoice_generate;
    
	switch (muladdmode) {
        case 0:        
            self->muladd_func_ptr = TrigChoice_postprocessing_ii;
            break;
        case 1:    
            self->muladd_func_ptr = TrigChoice_postprocessing_ai;
            break;
        case 2:    
            self->muladd_func_ptr = TrigChoice_postprocessing_revai;
            break;
        case 10:        
            self->muladd_func_ptr = TrigChoice_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = TrigChoice_postprocessing_aa;
            break;
        case 12:    
            self->muladd_func_ptr = TrigChoice_postprocessing_revaa;
            break;
        case 20:        
            self->muladd_func_ptr = TrigChoice_postprocessing_ireva;
            break;
        case 21:    
            self->muladd_func_ptr = TrigChoice_postprocessing_areva;
            break;
        case 22:    
            self->muladd_func_ptr = TrigChoice_postprocessing_revareva;
            break;
    }  
}

static void
TrigChoice_compute_next_data_frame(TrigChoice *self)
{
    (*self->proc_func_ptr)(self); 
    (*self->muladd_func_ptr)(self);
    Stream_setData(self->stream, self->data);
}

static int
TrigChoice_traverse(TrigChoice *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    return 0;
}

static int 
TrigChoice_clear(TrigChoice *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    return 0;
}

static void
TrigChoice_dealloc(TrigChoice* self)
{
    free(self->data);
    free(self->choice);
    TrigChoice_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * TrigChoice_deleteStream(TrigChoice *self) { DELETE_STREAM };

static PyObject *
TrigChoice_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    TrigChoice *self;
    self = (TrigChoice *)type->tp_alloc(type, 0);
    
    self->value = self->currentValue = 0.;
    self->time = 0.0;
    self->timeCount = 0;
    self->stepVal = 0.0;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, TrigChoice_compute_next_data_frame);
    self->mode_func_ptr = TrigChoice_setProcMode;
    return (PyObject *)self;
}

static int
TrigChoice_init(TrigChoice *self, PyObject *args, PyObject *kwds)
{
    float inittmp = 0.0;
    PyObject *inputtmp, *input_streamtmp, *choicetmp=NULL, *multmp=NULL, *addtmp=NULL;
    
    static char *kwlist[] = {"input", "choice", "port", "init", "mul", "add", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "OO|ffOO", kwlist, &inputtmp, &choicetmp, &self->time, &inittmp, &multmp, &addtmp))
        return -1; 
    
    Py_XDECREF(self->input);
    self->input = inputtmp;
    input_streamtmp = PyObject_CallMethod((PyObject *)self->input, "_getStream", NULL);
    Py_INCREF(input_streamtmp);
    Py_XDECREF(self->input_stream);
    self->input_stream = (Stream *)input_streamtmp;
    
    if (choicetmp) {
        PyObject_CallMethod((PyObject *)self, "setChoice", "O", choicetmp);
    }

    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }
    
    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }
    
    Py_INCREF(self->stream);
    PyObject_CallMethod(self->server, "addStream", "O", self->stream);
    
    srand((unsigned)(time(0)));
    self->value = self->currentValue = inittmp;
    self->timeStep = (int)(self->time * self->sr);
    
    (*self->mode_func_ptr)(self);
    
    TrigChoice_compute_next_data_frame((TrigChoice *)self);
    
    Py_INCREF(self);
    return 0;
}

static PyObject * TrigChoice_getServer(TrigChoice* self) { GET_SERVER };
static PyObject * TrigChoice_getStream(TrigChoice* self) { GET_STREAM };
static PyObject * TrigChoice_setMul(TrigChoice *self, PyObject *arg) { SET_MUL };	
static PyObject * TrigChoice_setAdd(TrigChoice *self, PyObject *arg) { SET_ADD };	
static PyObject * TrigChoice_setSub(TrigChoice *self, PyObject *arg) { SET_SUB };	
static PyObject * TrigChoice_setDiv(TrigChoice *self, PyObject *arg) { SET_DIV };	

static PyObject * TrigChoice_play(TrigChoice *self) { PLAY };
static PyObject * TrigChoice_out(TrigChoice *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * TrigChoice_stop(TrigChoice *self) { STOP };

static PyObject * TrigChoice_multiply(TrigChoice *self, PyObject *arg) { MULTIPLY };
static PyObject * TrigChoice_inplace_multiply(TrigChoice *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * TrigChoice_add(TrigChoice *self, PyObject *arg) { ADD };
static PyObject * TrigChoice_inplace_add(TrigChoice *self, PyObject *arg) { INPLACE_ADD };
static PyObject * TrigChoice_sub(TrigChoice *self, PyObject *arg) { SUB };
static PyObject * TrigChoice_inplace_sub(TrigChoice *self, PyObject *arg) { INPLACE_SUB };
static PyObject * TrigChoice_div(TrigChoice *self, PyObject *arg) { DIV };
static PyObject * TrigChoice_inplace_div(TrigChoice *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
TrigChoice_setChoice(TrigChoice *self, PyObject *arg)
{
    int i;
	PyObject *tmp;
	
	if (! PyList_Check(arg)) {
        PyErr_SetString(PyExc_TypeError, "The choice attribute must be a list.");
		Py_INCREF(Py_None);
		return Py_None;
	}

    tmp = arg;
    self->chSize = PyList_Size(tmp);
    self->choice = (float *)realloc(self->choice, self->chSize * sizeof(float));
    for (i=0; i<self->chSize; i++) {
        self->choice[i] = PyFloat_AS_DOUBLE(PyNumber_Float(PyList_GET_ITEM(tmp, i)));
    }
    
    (*self->mode_func_ptr)(self);
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyObject *
TrigChoice_setPort(TrigChoice *self, PyObject *arg)
{
	PyObject *tmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);
	
	tmp = arg;
	Py_INCREF(tmp);
	if (isNumber == 1) {
		self->time = PyFloat_AS_DOUBLE(PyNumber_Float(tmp));
        self->timeStep = (int)(self->time * self->sr);
	}
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyMemberDef TrigChoice_members[] = {
{"server", T_OBJECT_EX, offsetof(TrigChoice, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(TrigChoice, stream), 0, "Stream object."},
{"input", T_OBJECT_EX, offsetof(TrigChoice, input), 0, "Input sound object."},
{"mul", T_OBJECT_EX, offsetof(TrigChoice, mul), 0, "Mul factor."},
{"add", T_OBJECT_EX, offsetof(TrigChoice, add), 0, "Add factor."},
{NULL}  /* Sentinel */
};

static PyMethodDef TrigChoice_methods[] = {
{"getServer", (PyCFunction)TrigChoice_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)TrigChoice_getStream, METH_NOARGS, "Returns stream object."},
{"deleteStream", (PyCFunction)TrigChoice_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
{"play", (PyCFunction)TrigChoice_play, METH_NOARGS, "Starts computing without sending sound to soundcard."},
{"out", (PyCFunction)TrigChoice_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
{"stop", (PyCFunction)TrigChoice_stop, METH_NOARGS, "Stops computing."},
{"setChoice", (PyCFunction)TrigChoice_setChoice, METH_O, "Sets possible values."},
{"setPort", (PyCFunction)TrigChoice_setPort, METH_O, "Sets new portamento time."},
{"setMul", (PyCFunction)TrigChoice_setMul, METH_O, "Sets oscillator mul factor."},
{"setAdd", (PyCFunction)TrigChoice_setAdd, METH_O, "Sets oscillator add factor."},
{"setSub", (PyCFunction)TrigChoice_setSub, METH_O, "Sets inverse add factor."},
{"setDiv", (PyCFunction)TrigChoice_setDiv, METH_O, "Sets inverse mul factor."},
{NULL}  /* Sentinel */
};

static PyNumberMethods TrigChoice_as_number = {
(binaryfunc)TrigChoice_add,                         /*nb_add*/
(binaryfunc)TrigChoice_sub,                         /*nb_subtract*/
(binaryfunc)TrigChoice_multiply,                    /*nb_multiply*/
(binaryfunc)TrigChoice_div,                                              /*nb_divide*/
0,                                              /*nb_remainder*/
0,                                              /*nb_divmod*/
0,                                              /*nb_power*/
0,                                              /*nb_neg*/
0,                                              /*nb_pos*/
0,                                              /*(unaryfunc)array_abs,*/
0,                                              /*nb_nonzero*/
0,                                              /*nb_invert*/
0,                                              /*nb_lshift*/
0,                                              /*nb_rshift*/
0,                                              /*nb_and*/
0,                                              /*nb_xor*/
0,                                              /*nb_or*/
0,                                              /*nb_coerce*/
0,                                              /*nb_int*/
0,                                              /*nb_long*/
0,                                              /*nb_float*/
0,                                              /*nb_oct*/
0,                                              /*nb_hex*/
(binaryfunc)TrigChoice_inplace_add,                 /*inplace_add*/
(binaryfunc)TrigChoice_inplace_sub,                 /*inplace_subtract*/
(binaryfunc)TrigChoice_inplace_multiply,            /*inplace_multiply*/
(binaryfunc)TrigChoice_inplace_div,                                              /*inplace_divide*/
0,                                              /*inplace_remainder*/
0,                                              /*inplace_power*/
0,                                              /*inplace_lshift*/
0,                                              /*inplace_rshift*/
0,                                              /*inplace_and*/
0,                                              /*inplace_xor*/
0,                                              /*inplace_or*/
0,                                              /*nb_floor_divide*/
0,                                              /*nb_true_divide*/
0,                                              /*nb_inplace_floor_divide*/
0,                                              /*nb_inplace_true_divide*/
0,                                              /* nb_index */
};

PyTypeObject TrigChoiceType = {
PyObject_HEAD_INIT(NULL)
0,                                              /*ob_size*/
"_pyo.TrigChoice_base",                                   /*tp_name*/
sizeof(TrigChoice),                                 /*tp_basicsize*/
0,                                              /*tp_itemsize*/
(destructor)TrigChoice_dealloc,                     /*tp_dealloc*/
0,                                              /*tp_print*/
0,                                              /*tp_getattr*/
0,                                              /*tp_setattr*/
0,                                              /*tp_compare*/
0,                                              /*tp_repr*/
&TrigChoice_as_number,                              /*tp_as_number*/
0,                                              /*tp_as_sequence*/
0,                                              /*tp_as_mapping*/
0,                                              /*tp_hash */
0,                                              /*tp_call*/
0,                                              /*tp_str*/
0,                                              /*tp_getattro*/
0,                                              /*tp_setattro*/
0,                                              /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
"TrigChoice objects. Generates a new random value pick in a user choice on a trigger signal.",           /* tp_doc */
(traverseproc)TrigChoice_traverse,                  /* tp_traverse */
(inquiry)TrigChoice_clear,                          /* tp_clear */
0,                                              /* tp_richcompare */
0,                                              /* tp_weaklistoffset */
0,                                              /* tp_iter */
0,                                              /* tp_iternext */
TrigChoice_methods,                                 /* tp_methods */
TrigChoice_members,                                 /* tp_members */
0,                                              /* tp_getset */
0,                                              /* tp_base */
0,                                              /* tp_dict */
0,                                              /* tp_descr_get */
0,                                              /* tp_descr_set */
0,                                              /* tp_dictoffset */
(initproc)TrigChoice_init,                          /* tp_init */
0,                                              /* tp_alloc */
TrigChoice_new,                                     /* tp_new */
};

/*********************************************************************************************/
/* TrigFunc ********************************************************************************/
/*********************************************************************************************/
typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *func;
} TrigFunc;

static void
TrigFunc_generate(TrigFunc *self) {
    int i;
    float *in = Stream_getData((Stream *)self->input_stream);
    
    for (i=0; i<self->bufsize; i++) {
        if (in[i] == 1)
            PyObject_CallFunction((PyObject *)self->func, NULL);
    }
}

static void
TrigFunc_compute_next_data_frame(TrigFunc *self)
{
    TrigFunc_generate(self); 
}

static int
TrigFunc_traverse(TrigFunc *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    return 0;
}

static int 
TrigFunc_clear(TrigFunc *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    return 0;
}

static void
TrigFunc_dealloc(TrigFunc* self)
{
    free(self->data);
    TrigFunc_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * TrigFunc_deleteStream(TrigFunc *self) { DELETE_STREAM };

static PyObject *
TrigFunc_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    TrigFunc *self;
    self = (TrigFunc *)type->tp_alloc(type, 0);
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, TrigFunc_compute_next_data_frame);
    return (PyObject *)self;
}

static int
TrigFunc_init(TrigFunc *self, PyObject *args, PyObject *kwds)
{
    PyObject *inputtmp, *input_streamtmp, *functmp=NULL;
    
    static char *kwlist[] = {"input", "function", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "OO", kwlist, &inputtmp, &functmp))
        return -1; 
    
    Py_XDECREF(self->input);
    self->input = inputtmp;
    input_streamtmp = PyObject_CallMethod((PyObject *)self->input, "_getStream", NULL);
    Py_INCREF(input_streamtmp);
    Py_XDECREF(self->input_stream);
    self->input_stream = (Stream *)input_streamtmp;
    
    if (functmp) {
        PyObject_CallMethod((PyObject *)self, "setFunction", "O", functmp);
    }

    Py_INCREF(self->stream);
    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    TrigFunc_compute_next_data_frame((TrigFunc *)self);
    
    Py_INCREF(self);
    return 0;
}

static PyObject * TrigFunc_getServer(TrigFunc* self) { GET_SERVER };
static PyObject * TrigFunc_getStream(TrigFunc* self) { GET_STREAM };

static PyObject * TrigFunc_play(TrigFunc *self) { PLAY };
static PyObject * TrigFunc_stop(TrigFunc *self) { STOP };

static PyObject *
TrigFunc_setFunction(TrigFunc *self, PyObject *arg)
{
	PyObject *tmp;
	
	if (! PyFunction_Check(arg)) {
        PyErr_SetString(PyExc_TypeError, "The function attribute must be a function.");
		Py_INCREF(Py_None);
		return Py_None;
	}
    
    tmp = arg;
    Py_XDECREF(self->func);
    Py_INCREF(tmp);
    self->func = tmp;
  
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyMemberDef TrigFunc_members[] = {
{"server", T_OBJECT_EX, offsetof(TrigFunc, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(TrigFunc, stream), 0, "Stream object."},
{"input", T_OBJECT_EX, offsetof(TrigFunc, input), 0, "Input sound object."},
{NULL}  /* Sentinel */
};

static PyMethodDef TrigFunc_methods[] = {
{"getServer", (PyCFunction)TrigFunc_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)TrigFunc_getStream, METH_NOARGS, "Returns stream object."},
{"deleteStream", (PyCFunction)TrigFunc_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
{"play", (PyCFunction)TrigFunc_play, METH_NOARGS, "Starts computing without sending sound to soundcard."},
{"stop", (PyCFunction)TrigFunc_stop, METH_NOARGS, "Stops computing."},
{"setFunction", (PyCFunction)TrigFunc_setFunction, METH_O, "Sets function to be called."},
{NULL}  /* Sentinel */
};

PyTypeObject TrigFuncType = {
PyObject_HEAD_INIT(NULL)
0,                                              /*ob_size*/
"_pyo.TrigFunc_base",                                   /*tp_name*/
sizeof(TrigFunc),                                 /*tp_basicsize*/
0,                                              /*tp_itemsize*/
(destructor)TrigFunc_dealloc,                     /*tp_dealloc*/
0,                                              /*tp_print*/
0,                                              /*tp_getattr*/
0,                                              /*tp_setattr*/
0,                                              /*tp_compare*/
0,                                              /*tp_repr*/
0,                              /*tp_as_number*/
0,                                              /*tp_as_sequence*/
0,                                              /*tp_as_mapping*/
0,                                              /*tp_hash */
0,                                              /*tp_call*/
0,                                              /*tp_str*/
0,                                              /*tp_getattro*/
0,                                              /*tp_setattro*/
0,                                              /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
"TrigFunc objects. Called a function on a trigger signal.",           /* tp_doc */
(traverseproc)TrigFunc_traverse,                  /* tp_traverse */
(inquiry)TrigFunc_clear,                          /* tp_clear */
0,                                              /* tp_richcompare */
0,                                              /* tp_weaklistoffset */
0,                                              /* tp_iter */
0,                                              /* tp_iternext */
TrigFunc_methods,                                 /* tp_methods */
TrigFunc_members,                                 /* tp_members */
0,                                              /* tp_getset */
0,                                              /* tp_base */
0,                                              /* tp_dict */
0,                                              /* tp_descr_get */
0,                                              /* tp_descr_set */
0,                                              /* tp_dictoffset */
(initproc)TrigFunc_init,                          /* tp_init */
0,                                              /* tp_alloc */
TrigFunc_new,                                     /* tp_new */
};

/*********************************************************************************************/
/* TrigEnv *********************************************************************************/
/*********************************************************************************************/
typedef struct {
    pyo_audio_HEAD
    PyObject *table;
    PyObject *input;
    Stream *input_stream;
    PyObject *dur;
    Stream *dur_stream;
    int modebuffer[3];
    int active;
    float current_dur; // duration in samples
    float inc; // table size / current_dur
    double pointerPos; // reading position in sample
    float *trigsBuffer;
    float *tempTrigsBuffer;
} TrigEnv;

static void
TrigEnv_readframes_i(TrigEnv *self) {
    float fpart, x, x1;
    int i, ipart;
    float *in = Stream_getData((Stream *)self->input_stream);
    float *tablelist = TableStream_getData(self->table);
    int size = TableStream_getSize(self->table);
    
    for (i=0; i<self->bufsize; i++) {
        if (in[i] == 1) {
            float dur = PyFloat_AS_DOUBLE(self->dur);
            self->current_dur = self->sr * dur;
            self->inc = (float)size / self->current_dur;
            self->active = 1;
            self->pointerPos = 0.;
        }
        if (self->active == 1) {
            ipart = (int)self->pointerPos;
            fpart = self->pointerPos - ipart;
            x = tablelist[ipart];
            x1 = tablelist[ipart+1];
            self->data[i] = x + (x1 - x) * fpart;
            self->pointerPos += self->inc;
        }
        else
            self->data[i] = 0.;
        
        if (self->pointerPos > size && self->active == 1) {
            self->trigsBuffer[i] = 1.0;
            self->active = 0;
        }    
    }
}

static void
TrigEnv_readframes_a(TrigEnv *self) {
    float fpart, x, x1;
    int i, ipart;
    float *in = Stream_getData((Stream *)self->input_stream);
    float *dur_st = Stream_getData((Stream *)self->dur_stream);
    float *tablelist = TableStream_getData(self->table);
    int size = TableStream_getSize(self->table);
    
    for (i=0; i<self->bufsize; i++) {
        if (in[i] == 1) {
            float dur = dur_st[i];
            self->current_dur = self->sr * dur;
            self->inc = (float)size / self->current_dur;
            self->active = 1;
            self->pointerPos = 0.;
        }
        if (self->active == 1) {
            ipart = (int)self->pointerPos;
            fpart = self->pointerPos - ipart;
            x = tablelist[ipart];
            x1 = tablelist[ipart+1];
            self->data[i] = x + (x1 - x) * fpart;
            self->pointerPos += self->inc;
        }
        else
            self->data[i] = 0.;
        
        if (self->pointerPos > size && self->active == 1) {
            self->trigsBuffer[i] = 1.0;
            self->active = 0;
        }
    }
}

static void TrigEnv_postprocessing_ii(TrigEnv *self) { POST_PROCESSING_II };
static void TrigEnv_postprocessing_ai(TrigEnv *self) { POST_PROCESSING_AI };
static void TrigEnv_postprocessing_ia(TrigEnv *self) { POST_PROCESSING_IA };
static void TrigEnv_postprocessing_aa(TrigEnv *self) { POST_PROCESSING_AA };
static void TrigEnv_postprocessing_ireva(TrigEnv *self) { POST_PROCESSING_IREVA };
static void TrigEnv_postprocessing_areva(TrigEnv *self) { POST_PROCESSING_AREVA };
static void TrigEnv_postprocessing_revai(TrigEnv *self) { POST_PROCESSING_REVAI };
static void TrigEnv_postprocessing_revaa(TrigEnv *self) { POST_PROCESSING_REVAA };
static void TrigEnv_postprocessing_revareva(TrigEnv *self) { POST_PROCESSING_REVAREVA };

static void
TrigEnv_setProcMode(TrigEnv *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2];
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;
    
	switch (procmode) {
        case 0:        
            self->proc_func_ptr = TrigEnv_readframes_i;
            break;
        case 1:    
            self->proc_func_ptr = TrigEnv_readframes_a;
            break;
    } 
	switch (muladdmode) {
        case 0:        
            self->muladd_func_ptr = TrigEnv_postprocessing_ii;
            break;
        case 1:    
            self->muladd_func_ptr = TrigEnv_postprocessing_ai;
            break;
        case 2:    
            self->muladd_func_ptr = TrigEnv_postprocessing_revai;
            break;
        case 10:        
            self->muladd_func_ptr = TrigEnv_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = TrigEnv_postprocessing_aa;
            break;
        case 12:    
            self->muladd_func_ptr = TrigEnv_postprocessing_revaa;
            break;
        case 20:        
            self->muladd_func_ptr = TrigEnv_postprocessing_ireva;
            break;
        case 21:    
            self->muladd_func_ptr = TrigEnv_postprocessing_areva;
            break;
        case 22:    
            self->muladd_func_ptr = TrigEnv_postprocessing_revareva;
            break;
    } 
}

static void
TrigEnv_compute_next_data_frame(TrigEnv *self)
{
    (*self->proc_func_ptr)(self); 
    (*self->muladd_func_ptr)(self);
    Stream_setData(self->stream, self->data);
}

static int
TrigEnv_traverse(TrigEnv *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    Py_VISIT(self->table);
    Py_VISIT(self->dur);    
    Py_VISIT(self->dur_stream);    
    return 0;
}

static int 
TrigEnv_clear(TrigEnv *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    Py_CLEAR(self->table);
    Py_CLEAR(self->dur);    
    Py_CLEAR(self->dur_stream);    
    return 0;
}

static void
TrigEnv_dealloc(TrigEnv* self)
{
    free(self->data);
    free(self->tempTrigsBuffer);
    free(self->trigsBuffer);
    TrigEnv_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * TrigEnv_deleteStream(TrigEnv *self) { DELETE_STREAM };

static PyObject *
TrigEnv_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    TrigEnv *self;
    self = (TrigEnv *)type->tp_alloc(type, 0);
    
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;
    
    self->pointerPos = 0.;
    self->active = 0;
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, TrigEnv_compute_next_data_frame);
    self->mode_func_ptr = TrigEnv_setProcMode;

    self->dur = PyFloat_FromDouble(1.);
    self->current_dur = self->sr;

    return (PyObject *)self;
}

static int
TrigEnv_init(TrigEnv *self, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *tabletmp, *durtmp=NULL, *multmp=NULL, *addtmp=NULL;
    
    static char *kwlist[] = {"input", "table", "dur", "mul", "add", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "OO|OOO", kwlist, &inputtmp, &tabletmp, &durtmp, &multmp, &addtmp))
        return -1; 

    Py_XDECREF(self->input);
    self->input = inputtmp;
    input_streamtmp = PyObject_CallMethod((PyObject *)self->input, "_getStream", NULL);
    Py_INCREF(input_streamtmp);
    Py_XDECREF(self->input_stream);
    self->input_stream = (Stream *)input_streamtmp;
    
    Py_XDECREF(self->table);
    self->table = PyObject_CallMethod((PyObject *)tabletmp, "getTableStream", "");
    
    if (durtmp) {
        PyObject_CallMethod((PyObject *)self, "setDur", "O", durtmp);
    }
    
    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }
    
    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }
    
    Py_INCREF(self->stream);
    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    self->trigsBuffer = (float *)realloc(self->trigsBuffer, self->bufsize * sizeof(float));
    self->tempTrigsBuffer = (float *)realloc(self->tempTrigsBuffer, self->bufsize * sizeof(float));
    
    for (i=0; i<self->bufsize; i++) {
        self->trigsBuffer[i] = 0.0;
    }    
    
    (*self->mode_func_ptr)(self);
    
    TrigEnv_compute_next_data_frame((TrigEnv *)self);
    
    Py_INCREF(self);
    return 0;
}

static PyObject * TrigEnv_getServer(TrigEnv* self) { GET_SERVER };
static PyObject * TrigEnv_getStream(TrigEnv* self) { GET_STREAM };
static PyObject * TrigEnv_setMul(TrigEnv *self, PyObject *arg) { SET_MUL };	
static PyObject * TrigEnv_setAdd(TrigEnv *self, PyObject *arg) { SET_ADD };	
static PyObject * TrigEnv_setSub(TrigEnv *self, PyObject *arg) { SET_SUB };	
static PyObject * TrigEnv_setDiv(TrigEnv *self, PyObject *arg) { SET_DIV };	

static PyObject * TrigEnv_play(TrigEnv *self) { PLAY };
static PyObject * TrigEnv_out(TrigEnv *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * TrigEnv_stop(TrigEnv *self) { STOP };

static PyObject * TrigEnv_multiply(TrigEnv *self, PyObject *arg) { MULTIPLY };
static PyObject * TrigEnv_inplace_multiply(TrigEnv *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * TrigEnv_add(TrigEnv *self, PyObject *arg) { ADD };
static PyObject * TrigEnv_inplace_add(TrigEnv *self, PyObject *arg) { INPLACE_ADD };
static PyObject * TrigEnv_sub(TrigEnv *self, PyObject *arg) { SUB };
static PyObject * TrigEnv_inplace_sub(TrigEnv *self, PyObject *arg) { INPLACE_SUB };
static PyObject * TrigEnv_div(TrigEnv *self, PyObject *arg) { DIV };
static PyObject * TrigEnv_inplace_div(TrigEnv *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
TrigEnv_getTable(TrigEnv* self)
{
    Py_INCREF(self->table);
    return self->table;
};

static PyObject *
TrigEnv_setTable(TrigEnv *self, PyObject *arg)
{
	PyObject *tmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	tmp = arg;
	Py_DECREF(self->table);
    self->table = PyObject_CallMethod((PyObject *)tmp, "getTableStream", "");
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyObject *
TrigEnv_setDur(TrigEnv *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);
	
	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->dur);
	if (isNumber == 1) {
		self->dur = PyNumber_Float(tmp);
        self->modebuffer[2] = 0;
	}
	else {
		self->dur = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->dur, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->dur_stream);
        self->dur_stream = (Stream *)streamtmp;
		self->modebuffer[2] = 1;
	}
    
    (*self->mode_func_ptr)(self);
    
	Py_INCREF(Py_None);
	return Py_None;
}	

float *
TrigEnv_getTrigsBuffer(TrigEnv *self)
{
    int i;
    for (i=0; i<self->bufsize; i++) {
        self->tempTrigsBuffer[i] = self->trigsBuffer[i];
        self->trigsBuffer[i] = 0.0;
    }    
    return (float *)self->tempTrigsBuffer;
}    

static PyMemberDef TrigEnv_members[] = {
{"server", T_OBJECT_EX, offsetof(TrigEnv, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(TrigEnv, stream), 0, "Stream object."},
{"table", T_OBJECT_EX, offsetof(TrigEnv, table), 0, "Envelope table."},
{"dur", T_OBJECT_EX, offsetof(TrigEnv, dur), 0, "Envelope duration in seconds."},
{"mul", T_OBJECT_EX, offsetof(TrigEnv, mul), 0, "Mul factor."},
{"add", T_OBJECT_EX, offsetof(TrigEnv, add), 0, "Add factor."},
{NULL}  /* Sentinel */
};

static PyMethodDef TrigEnv_methods[] = {
{"getTable", (PyCFunction)TrigEnv_getTable, METH_NOARGS, "Returns waveform table object."},
{"getServer", (PyCFunction)TrigEnv_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)TrigEnv_getStream, METH_NOARGS, "Returns stream object."},
{"deleteStream", (PyCFunction)TrigEnv_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
{"play", (PyCFunction)TrigEnv_play, METH_NOARGS, "Starts computing without sending sound to soundcard."},
{"out", (PyCFunction)TrigEnv_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
{"stop", (PyCFunction)TrigEnv_stop, METH_NOARGS, "Stops computing."},
{"setTable", (PyCFunction)TrigEnv_setTable, METH_O, "Sets envelope table."},
{"setDur", (PyCFunction)TrigEnv_setDur, METH_O, "Sets envelope duration in second."},
{"setMul", (PyCFunction)TrigEnv_setMul, METH_O, "Sets mul factor."},
{"setAdd", (PyCFunction)TrigEnv_setAdd, METH_O, "Sets add factor."},
{"setSub", (PyCFunction)TrigEnv_setSub, METH_O, "Sets inverse add factor."},
{"setDiv", (PyCFunction)TrigEnv_setDiv, METH_O, "Sets inverse mul factor."},
{NULL}  /* Sentinel */
};

static PyNumberMethods TrigEnv_as_number = {
(binaryfunc)TrigEnv_add,                      /*nb_add*/
(binaryfunc)TrigEnv_sub,                 /*nb_subtract*/
(binaryfunc)TrigEnv_multiply,                 /*nb_multiply*/
(binaryfunc)TrigEnv_div,                   /*nb_divide*/
0,                /*nb_remainder*/
0,                   /*nb_divmod*/
0,                   /*nb_power*/
0,                  /*nb_neg*/
0,                /*nb_pos*/
0,                  /*(unaryfunc)array_abs,*/
0,                    /*nb_nonzero*/
0,                    /*nb_invert*/
0,               /*nb_lshift*/
0,              /*nb_rshift*/
0,              /*nb_and*/
0,              /*nb_xor*/
0,               /*nb_or*/
0,                                          /*nb_coerce*/
0,                       /*nb_int*/
0,                      /*nb_long*/
0,                     /*nb_float*/
0,                       /*nb_oct*/
0,                       /*nb_hex*/
(binaryfunc)TrigEnv_inplace_add,              /*inplace_add*/
(binaryfunc)TrigEnv_inplace_sub,         /*inplace_subtract*/
(binaryfunc)TrigEnv_inplace_multiply,         /*inplace_multiply*/
(binaryfunc)TrigEnv_inplace_div,           /*inplace_divide*/
0,        /*inplace_remainder*/
0,           /*inplace_power*/
0,       /*inplace_lshift*/
0,      /*inplace_rshift*/
0,      /*inplace_and*/
0,      /*inplace_xor*/
0,       /*inplace_or*/
0,             /*nb_floor_divide*/
0,              /*nb_true_divide*/
0,     /*nb_inplace_floor_divide*/
0,      /*nb_inplace_true_divide*/
0,                     /* nb_index */
};

PyTypeObject TrigEnvType = {
PyObject_HEAD_INIT(NULL)
0,                         /*ob_size*/
"_pyo.TrigEnv_base",         /*tp_name*/
sizeof(TrigEnv),         /*tp_basicsize*/
0,                         /*tp_itemsize*/
(destructor)TrigEnv_dealloc, /*tp_dealloc*/
0,                         /*tp_print*/
0,                         /*tp_getattr*/
0,                         /*tp_setattr*/
0,                         /*tp_compare*/
0,                         /*tp_repr*/
&TrigEnv_as_number,             /*tp_as_number*/
0,                         /*tp_as_sequence*/
0,                         /*tp_as_mapping*/
0,                         /*tp_hash */
0,                         /*tp_call*/
0,                         /*tp_str*/
0,                         /*tp_getattro*/
0,                         /*tp_setattro*/
0,                         /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
"TrigEnv objects. Starts an envelope on a trigger signal.",           /* tp_doc */
(traverseproc)TrigEnv_traverse,   /* tp_traverse */
(inquiry)TrigEnv_clear,           /* tp_clear */
0,		               /* tp_richcompare */
0,		               /* tp_weaklistoffset */
0,		               /* tp_iter */
0,		               /* tp_iternext */
TrigEnv_methods,             /* tp_methods */
TrigEnv_members,             /* tp_members */
0,                      /* tp_getset */
0,                         /* tp_base */
0,                         /* tp_dict */
0,                         /* tp_descr_get */
0,                         /* tp_descr_set */
0,                         /* tp_dictoffset */
(initproc)TrigEnv_init,      /* tp_init */
0,                         /* tp_alloc */
TrigEnv_new,                 /* tp_new */
};

/************************************************************************************************/
/* TrigEnv trig streamer */
/************************************************************************************************/
typedef struct {
    pyo_audio_HEAD
    TrigEnv *mainReader;
} TrigEnvTrig;

static void
TrigEnvTrig_compute_next_data_frame(TrigEnvTrig *self)
{
    int i;
    float *tmp;
    tmp = TrigEnv_getTrigsBuffer((TrigEnv *)self->mainReader);
    for (i=0; i<self->bufsize; i++) {
        self->data[i] = tmp[i];
    }    
    Stream_setData(self->stream, self->data);
}

static int
TrigEnvTrig_traverse(TrigEnvTrig *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->mainReader);
    return 0;
}

static int 
TrigEnvTrig_clear(TrigEnvTrig *self)
{
    pyo_CLEAR
    Py_CLEAR(self->mainReader);    
    return 0;
}

static void
TrigEnvTrig_dealloc(TrigEnvTrig* self)
{
    free(self->data);
    TrigEnvTrig_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * TrigEnvTrig_deleteStream(TrigEnvTrig *self) { DELETE_STREAM };

static PyObject *
TrigEnvTrig_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    TrigEnvTrig *self;
    self = (TrigEnvTrig *)type->tp_alloc(type, 0);
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, TrigEnvTrig_compute_next_data_frame);
    
    return (PyObject *)self;
}

static int
TrigEnvTrig_init(TrigEnvTrig *self, PyObject *args, PyObject *kwds)
{
    PyObject *maintmp=NULL;
    
    static char *kwlist[] = {"mainReader", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O", kwlist, &maintmp))
        return -1; 
    
    Py_XDECREF(self->mainReader);
    Py_INCREF(maintmp);
    self->mainReader = (TrigEnv *)maintmp;
    
    Py_INCREF(self->stream);
    PyObject_CallMethod(self->server, "addStream", "O", self->stream);
    
    TrigEnvTrig_compute_next_data_frame((TrigEnvTrig *)self);
    
    Py_INCREF(self);
    return 0;
}

static PyObject * TrigEnvTrig_getServer(TrigEnvTrig* self) { GET_SERVER };
static PyObject * TrigEnvTrig_getStream(TrigEnvTrig* self) { GET_STREAM };

static PyObject * TrigEnvTrig_play(TrigEnvTrig *self) { PLAY };
static PyObject * TrigEnvTrig_stop(TrigEnvTrig *self) { STOP };

static PyMemberDef TrigEnvTrig_members[] = {
{"server", T_OBJECT_EX, offsetof(TrigEnvTrig, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(TrigEnvTrig, stream), 0, "Stream object."},
{NULL}  /* Sentinel */
};

static PyMethodDef TrigEnvTrig_methods[] = {
{"getServer", (PyCFunction)TrigEnvTrig_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)TrigEnvTrig_getStream, METH_NOARGS, "Returns stream object."},
{"deleteStream", (PyCFunction)TrigEnvTrig_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
{"play", (PyCFunction)TrigEnvTrig_play, METH_NOARGS, "Starts computing without sending sound to soundcard."},
{"stop", (PyCFunction)TrigEnvTrig_stop, METH_NOARGS, "Stops computing."},
{NULL}  /* Sentinel */
};

PyTypeObject TrigEnvTrigType = {
PyObject_HEAD_INIT(NULL)
0,                         /*ob_size*/
"_pyo.TrigEnvTrig_base",         /*tp_name*/
sizeof(TrigEnvTrig),         /*tp_basicsize*/
0,                         /*tp_itemsize*/
(destructor)TrigEnvTrig_dealloc, /*tp_dealloc*/
0,                         /*tp_print*/
0,                         /*tp_getattr*/
0,                         /*tp_setattr*/
0,                         /*tp_compare*/
0,                         /*tp_repr*/
0,             /*tp_as_number*/
0,                         /*tp_as_sequence*/
0,                         /*tp_as_mapping*/
0,                         /*tp_hash */
0,                         /*tp_call*/
0,                         /*tp_str*/
0,                         /*tp_getattro*/
0,                         /*tp_setattro*/
0,                         /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES,  /*tp_flags*/
"TrigEnvTrig objects. Sends trigger at the end of playback.",           /* tp_doc */
(traverseproc)TrigEnvTrig_traverse,   /* tp_traverse */
(inquiry)TrigEnvTrig_clear,           /* tp_clear */
0,		               /* tp_richcompare */
0,		               /* tp_weaklistoffset */
0,		               /* tp_iter */
0,		               /* tp_iternext */
TrigEnvTrig_methods,             /* tp_methods */
TrigEnvTrig_members,             /* tp_members */
0,                      /* tp_getset */
0,                         /* tp_base */
0,                         /* tp_dict */
0,                         /* tp_descr_get */
0,                         /* tp_descr_set */
0,                         /* tp_dictoffset */
(initproc)TrigEnvTrig_init,      /* tp_init */
0,                         /* tp_alloc */
TrigEnvTrig_new,                 /* tp_new */
};

/****************/
/**** TrigXnoise *****/
/****************/
typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *x1;
    PyObject *x2;
    Stream *x1_stream;
    Stream *x2_stream;
    float (*type_func_ptr)();
    float xx1;
    float xx2;
    int type;
    float value;
    float lastPoissonX1;
    int poisson_tab;
    float poisson_buffer[2000];
    float walkerValue;
    float loop_buffer[15];
    int loopChoice;
    int loopCountPlay;
    int loopTime;
    int loopCountRec;
    int loopLen;
    int loopStop;
    int modebuffer[4]; // need at least 2 slots for mul & add 
} TrigXnoise;

// no parameter
static float
TrigXnoise_uniform(TrigXnoise *self) {
    return RANDOM_UNIFORM;    
}

static float
TrigXnoise_linear_min(TrigXnoise *self) {
    float a = RANDOM_UNIFORM;    
    float b = RANDOM_UNIFORM;
    if (a < b) return a;
    else return b;
}

static float
TrigXnoise_linear_max(TrigXnoise *self) {
    float a = RANDOM_UNIFORM;    
    float b = RANDOM_UNIFORM;
    if (a > b) return a;
    else return b;
}

static float
TrigXnoise_triangle(TrigXnoise *self) {
    float a = RANDOM_UNIFORM;    
    float b = RANDOM_UNIFORM;
    return ((a + b) * 0.5);
}

// x1 = slope
static float
TrigXnoise_expon_min(TrigXnoise *self) {
    if (self->xx1 <= 0.0) self->xx1 = 0.00001;
    float val = -logf(RANDOM_UNIFORM) / self->xx1;    
    if (val < 0.0) return 0.0;
    else if (val > 1.0) return 1.0;
    else return val;
}

static float
TrigXnoise_expon_max(TrigXnoise *self) {
    if (self->xx1 <= 0.0) self->xx1 = 0.00001;
    float val = 1.0 - (-logf(RANDOM_UNIFORM) / self->xx1);    
    if (val < 0.0) return 0.0;
    else if (val > 1.0) return 1.0;
    else return val;
}

// x1 = bandwidth
static float
TrigXnoise_biexpon(TrigXnoise *self) {
    float polar, val;
    if (self->xx1 <= 0.0) self->xx1 = 0.00001;
    float sum = RANDOM_UNIFORM * 2.0;
    
    if (sum > 1.0) {
        polar = -1;
        sum = 2.0 - sum;
    }
    else
        polar = 1;
    
    val = 0.5 * (polar * logf(sum) / self->xx1) + 0.5;
    
    if (val < 0.0) return 0.0;
    else if (val > 1.0) return 1.0;
    else return val;
}

static float
TrigXnoise_cauchy(TrigXnoise *self) {
    float rnd, val, dir;
    do {
        rnd = RANDOM_UNIFORM;
    }
    while (rnd == 0.5);
    
    if (rand() < (RAND_MAX / 2))
        dir = -1;
    else
        dir = 1;
    
    val = 0.5 * (tanf(rnd) * self->xx1 * dir) + 0.5;
    
    if (val < 0.0) return 0.0;
    else if (val > 1.0) return 1.0;
    else return val;
}

// x1 = locator, x2 = shape
static float
TrigXnoise_weibull(TrigXnoise *self) {
    float rnd, val;
    if (self->xx2 <= 0.0) self->xx2 = 0.00001;
    
    rnd = 1.0 / (1.0 - RANDOM_UNIFORM);
    val = self->xx1 * powf(logf(rnd), (1.0 / self->xx2));
    
    if (val < 0.0) return 0.0;
    else if (val > 1.0) return 1.0;
    else return val;
}

// x1 = locator, x2 = bandwidth
static float
TrigXnoise_gaussian(TrigXnoise *self) {
    float rnd, val;
    
    rnd = (RANDOM_UNIFORM + RANDOM_UNIFORM + RANDOM_UNIFORM + RANDOM_UNIFORM + RANDOM_UNIFORM + RANDOM_UNIFORM);
    val = (self->xx2 * (rnd - 3.0) * 0.33 + self->xx1);
    
    if (val < 0.0) return 0.0;
    else if (val > 1.0) return 1.0;
    else return val;
}

// x1 = gravity center, x2 = compress/expand
static float
TrigXnoise_poisson(TrigXnoise *self) {
    int i, j, factorial;
    long tot;
    float val;
    if (self->xx1 < 0.1) self->xx1 = 0.1;
    if (self->xx2 < 0.1) self->xx2 = 0.1;
    
    if (self->xx1 != self->lastPoissonX1) {
        self->lastPoissonX1 = self->xx1;
        self->poisson_tab = 0;
        factorial = 1;
        for (i=1; i<12; i++) {
            factorial *= i;
            tot = (long)(1000.0 * (powf(2.7182818, -self->xx1) * powf(self->xx1, i) / factorial));
            for (j=0; j<tot; j++) {
                self->poisson_buffer[self->poisson_tab] = i;
                self->poisson_tab++;
            }
        }
    }
    val = self->poisson_buffer[rand() % self->poisson_tab] / 12.0 * self->xx2;
    
    if (val < 0.0) return 0.0;
    else if (val > 1.0) return 1.0;
    else return val;
}

// x1 = max value, x2 = max step
static float
TrigXnoise_walker(TrigXnoise *self) {
    int modulo, dir;
    
    if (self->xx2 < 0.002) self->xx2 = 0.002;
    
    modulo = (int)(self->xx2 * 1000.0);
    dir = rand() % 2;
    
    if (dir == 0)
        self->walkerValue = self->walkerValue + (((rand() % modulo) - (modulo / 2)) * 0.001);
    else
        self->walkerValue = self->walkerValue - (((rand() % modulo) - (modulo / 2)) * 0.001);
    
    if (self->walkerValue > self->xx1)
        self->walkerValue = self->xx1;
    if (self->walkerValue < 0.0)
        self->walkerValue = 0.0;
    
    return self->walkerValue;
}

// x1 = max value, x2 = max step
static float
TrigXnoise_loopseg(TrigXnoise *self) {
    int modulo, dir;
    
    if (self->loopChoice == 0) {
        
        self->loopCountPlay = self->loopTime = 0;
        
        if (self->xx2 < 0.002) self->xx2 = 0.002;
        
        modulo = (int)(self->xx2 * 1000.0);
        dir = rand() % 2;
        
        if (dir == 0)
            self->walkerValue = self->walkerValue + (((rand() % modulo) - (modulo / 2)) * 0.001);
        else
            self->walkerValue = self->walkerValue - (((rand() % modulo) - (modulo / 2)) * 0.001);
        
        if (self->walkerValue > self->xx1)
            self->walkerValue = self->xx1;
        if (self->walkerValue < 0.0)
            self->walkerValue = 0.0;
        
        self->loop_buffer[self->loopCountRec++] = self->walkerValue;
        
        if (self->loopCountRec < self->loopLen)
            self->loopChoice = 0;
        else {
            self->loopChoice = 1;
            self->loopStop = (rand() % 4) + 1;
        }
    }
    else {
        self->loopCountRec = 0;
        
        self->walkerValue = self->loop_buffer[self->loopCountPlay++];
        
        if (self->loopCountPlay < self->loopLen)
            self->loopChoice = 1;
        else {
            self->loopCountPlay = 0;
            self->loopTime++;
        }
        
        if (self->loopTime == self->loopStop) {
            self->loopChoice = 0;
            self->loopLen = (rand() % 10) + 3;
        }
    }
    
    return self->walkerValue;
}

static void
TrigXnoise_generate_ii(TrigXnoise *self) {
    int i;
    float *in = Stream_getData((Stream *)self->input_stream);
    self->xx1 = PyFloat_AS_DOUBLE(self->x1);
    self->xx2 = PyFloat_AS_DOUBLE(self->x2);
    
    for (i=0; i<self->bufsize; i++) {
        if (in[i] == 1)
            self->value = (*self->type_func_ptr)(self);
        self->data[i] = self->value;
    }
}

static void
TrigXnoise_generate_ai(TrigXnoise *self) {
    int i;
    float *in = Stream_getData((Stream *)self->input_stream);
    float *x1 = Stream_getData((Stream *)self->x1_stream);
    self->xx2 = PyFloat_AS_DOUBLE(self->x2);
    
    for (i=0; i<self->bufsize; i++) {
        if (in[i] == 1) {
            self->xx1 = x1[i];
            self->value = (*self->type_func_ptr)(self);
        }
        self->data[i] = self->value;
    }
}

static void
TrigXnoise_generate_ia(TrigXnoise *self) {
    int i;
    float *in = Stream_getData((Stream *)self->input_stream);
    self->xx1 = PyFloat_AS_DOUBLE(self->x1);
    float *x2 = Stream_getData((Stream *)self->x2_stream);
    
    for (i=0; i<self->bufsize; i++) {
        if (in[i] == 1) {
            self->xx2 = x2[i];
            self->value = (*self->type_func_ptr)(self);
        }
        self->data[i] = self->value;
    }
}

static void
TrigXnoise_generate_aa(TrigXnoise *self) {
    int i;
    float *in = Stream_getData((Stream *)self->input_stream);
    float *x1 = Stream_getData((Stream *)self->x1_stream);
    float *x2 = Stream_getData((Stream *)self->x2_stream);
    
    for (i=0; i<self->bufsize; i++) {
        if (in[i] == 1) {
            self->xx1 = x1[i];
            self->xx2 = x2[i];
            self->value = (*self->type_func_ptr)(self);
        }
        self->data[i] = self->value;
    }
}

static void TrigXnoise_postprocessing_ii(TrigXnoise *self) { POST_PROCESSING_II };
static void TrigXnoise_postprocessing_ai(TrigXnoise *self) { POST_PROCESSING_AI };
static void TrigXnoise_postprocessing_ia(TrigXnoise *self) { POST_PROCESSING_IA };
static void TrigXnoise_postprocessing_aa(TrigXnoise *self) { POST_PROCESSING_AA };
static void TrigXnoise_postprocessing_ireva(TrigXnoise *self) { POST_PROCESSING_IREVA };
static void TrigXnoise_postprocessing_areva(TrigXnoise *self) { POST_PROCESSING_AREVA };
static void TrigXnoise_postprocessing_revai(TrigXnoise *self) { POST_PROCESSING_REVAI };
static void TrigXnoise_postprocessing_revaa(TrigXnoise *self) { POST_PROCESSING_REVAA };
static void TrigXnoise_postprocessing_revareva(TrigXnoise *self) { POST_PROCESSING_REVAREVA };

static void
TrigXnoise_setRandomType(TrigXnoise *self)
{
    
    switch (self->type) {            
        case 0:
            self->type_func_ptr = TrigXnoise_uniform;
            break;
        case 1:
            self->type_func_ptr = TrigXnoise_linear_min;
            break;
        case 2:
            self->type_func_ptr = TrigXnoise_linear_max;
            break;
        case 3:
            self->type_func_ptr = TrigXnoise_triangle;
            break;
        case 4:
            self->type_func_ptr = TrigXnoise_expon_min;
            break;
        case 5:
            self->type_func_ptr = TrigXnoise_expon_max;
            break;
        case 6:
            self->type_func_ptr = TrigXnoise_biexpon;
            break;
        case 7:
            self->type_func_ptr = TrigXnoise_cauchy;
            break;
        case 8:
            self->type_func_ptr = TrigXnoise_weibull;
            break;
        case 9:
            self->type_func_ptr = TrigXnoise_gaussian;
            break;
        case 10:
            self->type_func_ptr = TrigXnoise_poisson;
            break;
        case 11:
            self->type_func_ptr = TrigXnoise_walker;
            break;
        case 12:
            self->type_func_ptr = TrigXnoise_loopseg;
            break;
    }        
}

static void
TrigXnoise_setProcMode(TrigXnoise *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2] + self->modebuffer[3] * 10;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;
    
	switch (procmode) {
        case 0:    
            self->proc_func_ptr = TrigXnoise_generate_ii;
            break;
        case 1:    
            self->proc_func_ptr = TrigXnoise_generate_ai;
            break;
        case 10:    
            self->proc_func_ptr = TrigXnoise_generate_ia;
            break;
        case 11:    
            self->proc_func_ptr = TrigXnoise_generate_aa;
            break;
    } 
	switch (muladdmode) {
        case 0:        
            self->muladd_func_ptr = TrigXnoise_postprocessing_ii;
            break;
        case 1:    
            self->muladd_func_ptr = TrigXnoise_postprocessing_ai;
            break;
        case 2:    
            self->muladd_func_ptr = TrigXnoise_postprocessing_revai;
            break;
        case 10:        
            self->muladd_func_ptr = TrigXnoise_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = TrigXnoise_postprocessing_aa;
            break;
        case 12:    
            self->muladd_func_ptr = TrigXnoise_postprocessing_revaa;
            break;
        case 20:        
            self->muladd_func_ptr = TrigXnoise_postprocessing_ireva;
            break;
        case 21:    
            self->muladd_func_ptr = TrigXnoise_postprocessing_areva;
            break;
        case 22:    
            self->muladd_func_ptr = TrigXnoise_postprocessing_revareva;
            break;
    }  
}

static void
TrigXnoise_compute_next_data_frame(TrigXnoise *self)
{
    (*self->proc_func_ptr)(self); 
    (*self->muladd_func_ptr)(self);
    Stream_setData(self->stream, self->data);
}

static int
TrigXnoise_traverse(TrigXnoise *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    Py_VISIT(self->x1);    
    Py_VISIT(self->x1_stream);    
    Py_VISIT(self->x2);    
    Py_VISIT(self->x2_stream);    
    return 0;
}

static int 
TrigXnoise_clear(TrigXnoise *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    Py_CLEAR(self->x1);    
    Py_CLEAR(self->x1_stream);    
    Py_CLEAR(self->x2);    
    Py_CLEAR(self->x2_stream);    
    return 0;
}

static void
TrigXnoise_dealloc(TrigXnoise* self)
{
    free(self->data);
    TrigXnoise_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * TrigXnoise_deleteStream(TrigXnoise *self) { DELETE_STREAM };

static PyObject *
TrigXnoise_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    TrigXnoise *self;
    self = (TrigXnoise *)type->tp_alloc(type, 0);
    
    srand((unsigned)(time(0)));
    
    self->x1 = PyFloat_FromDouble(0.5);
    self->x2 = PyFloat_FromDouble(0.5);
    self->xx1 = self->xx2 = self->walkerValue = 0.5;
    self->value = 0.0;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;
	self->modebuffer[3] = 0;
    
    self->poisson_tab = 0;
    self->lastPoissonX1 = -99.0;
    for (i=0; i<2000; i++) {
        self->poisson_buffer[i] = 0.0;
    }
    for (i=0; i<15; i++) {
        self->loop_buffer[i] = 0.0;
    }
    self->loopChoice = self->loopCountPlay = self->loopTime = self->loopCountRec = self->loopStop = 0;    
    self->loopLen = (rand() % 10) + 3;
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, TrigXnoise_compute_next_data_frame);
    self->mode_func_ptr = TrigXnoise_setProcMode;
    return (PyObject *)self;
}

static int
TrigXnoise_init(TrigXnoise *self, PyObject *args, PyObject *kwds)
{
    PyObject *inputtmp, *input_streamtmp, *x1tmp=NULL, *x2tmp=NULL, *multmp=NULL, *addtmp=NULL;
    
    static char *kwlist[] = {"input", "type", "x1", "x2", "mul", "add", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|iOOOO", kwlist, &inputtmp, &self->type, &x1tmp, &x2tmp, &multmp, &addtmp))
        return -1; 

    Py_XDECREF(self->input);
    self->input = inputtmp;
    input_streamtmp = PyObject_CallMethod((PyObject *)self->input, "_getStream", NULL);
    Py_INCREF(input_streamtmp);
    Py_XDECREF(self->input_stream);
    self->input_stream = (Stream *)input_streamtmp;
    
    if (x1tmp) {
        PyObject_CallMethod((PyObject *)self, "setX1", "O", x1tmp);
    }
    
    if (x2tmp) {
        PyObject_CallMethod((PyObject *)self, "setX2", "O", x2tmp);
    }

    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }
    
    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }
    
    Py_INCREF(self->stream);
    PyObject_CallMethod(self->server, "addStream", "O", self->stream);
    
    TrigXnoise_setRandomType(self);
    
    (*self->mode_func_ptr)(self);
    
    TrigXnoise_compute_next_data_frame((TrigXnoise *)self);
    
    Py_INCREF(self);
    return 0;
}

static PyObject * TrigXnoise_getServer(TrigXnoise* self) { GET_SERVER };
static PyObject * TrigXnoise_getStream(TrigXnoise* self) { GET_STREAM };
static PyObject * TrigXnoise_setMul(TrigXnoise *self, PyObject *arg) { SET_MUL };	
static PyObject * TrigXnoise_setAdd(TrigXnoise *self, PyObject *arg) { SET_ADD };	
static PyObject * TrigXnoise_setSub(TrigXnoise *self, PyObject *arg) { SET_SUB };	
static PyObject * TrigXnoise_setDiv(TrigXnoise *self, PyObject *arg) { SET_DIV };	

static PyObject * TrigXnoise_play(TrigXnoise *self) { PLAY };
static PyObject * TrigXnoise_out(TrigXnoise *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * TrigXnoise_stop(TrigXnoise *self) { STOP };

static PyObject * TrigXnoise_multiply(TrigXnoise *self, PyObject *arg) { MULTIPLY };
static PyObject * TrigXnoise_inplace_multiply(TrigXnoise *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * TrigXnoise_add(TrigXnoise *self, PyObject *arg) { ADD };
static PyObject * TrigXnoise_inplace_add(TrigXnoise *self, PyObject *arg) { INPLACE_ADD };
static PyObject * TrigXnoise_sub(TrigXnoise *self, PyObject *arg) { SUB };
static PyObject * TrigXnoise_inplace_sub(TrigXnoise *self, PyObject *arg) { INPLACE_SUB };
static PyObject * TrigXnoise_div(TrigXnoise *self, PyObject *arg) { DIV };
static PyObject * TrigXnoise_inplace_div(TrigXnoise *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
TrigXnoise_setType(TrigXnoise *self, PyObject *arg)
{	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyInt_Check(arg);
	
	if (isNumber == 1) {
		self->type = PyInt_AsLong(arg);
        TrigXnoise_setRandomType(self);
	}
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyObject *
TrigXnoise_setX1(TrigXnoise *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);
	
	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->x1);
	if (isNumber == 1) {
		self->x1 = PyNumber_Float(tmp);
        self->modebuffer[2] = 0;
	}
	else {
		self->x1 = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->x1, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->x1_stream);
        self->x1_stream = (Stream *)streamtmp;
		self->modebuffer[2] = 1;
	}
    
    (*self->mode_func_ptr)(self);
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyObject *
TrigXnoise_setX2(TrigXnoise *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);
	
	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->x2);
	if (isNumber == 1) {
		self->x2 = PyNumber_Float(tmp);
        self->modebuffer[3] = 0;
	}
	else {
		self->x2 = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->x2, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->x2_stream);
        self->x2_stream = (Stream *)streamtmp;
		self->modebuffer[3] = 1;
	}
    
    (*self->mode_func_ptr)(self);
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyMemberDef TrigXnoise_members[] = {
    {"server", T_OBJECT_EX, offsetof(TrigXnoise, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(TrigXnoise, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(TrigXnoise, input), 0, "Trigger input."},
    {"x1", T_OBJECT_EX, offsetof(TrigXnoise, x1), 0, "first param."},
    {"x2", T_OBJECT_EX, offsetof(TrigXnoise, x2), 0, "second param."},
    {"mul", T_OBJECT_EX, offsetof(TrigXnoise, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(TrigXnoise, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef TrigXnoise_methods[] = {
    {"getServer", (PyCFunction)TrigXnoise_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)TrigXnoise_getStream, METH_NOARGS, "Returns stream object."},
    {"deleteStream", (PyCFunction)TrigXnoise_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
    {"play", (PyCFunction)TrigXnoise_play, METH_NOARGS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)TrigXnoise_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)TrigXnoise_stop, METH_NOARGS, "Stops computing."},
    {"setType", (PyCFunction)TrigXnoise_setType, METH_O, "Sets distribution type."},
    {"setX1", (PyCFunction)TrigXnoise_setX1, METH_O, "Sets first param."},
    {"setX2", (PyCFunction)TrigXnoise_setX2, METH_O, "Sets second param."},
    {"setMul", (PyCFunction)TrigXnoise_setMul, METH_O, "Sets oscillator mul factor."},
    {"setAdd", (PyCFunction)TrigXnoise_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)TrigXnoise_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)TrigXnoise_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods TrigXnoise_as_number = {
    (binaryfunc)TrigXnoise_add,                         /*nb_add*/
    (binaryfunc)TrigXnoise_sub,                         /*nb_subtract*/
    (binaryfunc)TrigXnoise_multiply,                    /*nb_multiply*/
    (binaryfunc)TrigXnoise_div,                                              /*nb_divide*/
    0,                                              /*nb_remainder*/
    0,                                              /*nb_divmod*/
    0,                                              /*nb_power*/
    0,                                              /*nb_neg*/
    0,                                              /*nb_pos*/
    0,                                              /*(unaryfunc)array_abs,*/
    0,                                              /*nb_nonzero*/
    0,                                              /*nb_invert*/
    0,                                              /*nb_lshift*/
    0,                                              /*nb_rshift*/
    0,                                              /*nb_and*/
    0,                                              /*nb_xor*/
    0,                                              /*nb_or*/
    0,                                              /*nb_coerce*/
    0,                                              /*nb_int*/
    0,                                              /*nb_long*/
    0,                                              /*nb_float*/
    0,                                              /*nb_oct*/
    0,                                              /*nb_hex*/
    (binaryfunc)TrigXnoise_inplace_add,                 /*inplace_add*/
    (binaryfunc)TrigXnoise_inplace_sub,                 /*inplace_subtract*/
    (binaryfunc)TrigXnoise_inplace_multiply,            /*inplace_multiply*/
    (binaryfunc)TrigXnoise_inplace_div,                                              /*inplace_divide*/
    0,                                              /*inplace_remainder*/
    0,                                              /*inplace_power*/
    0,                                              /*inplace_lshift*/
    0,                                              /*inplace_rshift*/
    0,                                              /*inplace_and*/
    0,                                              /*inplace_xor*/
    0,                                              /*inplace_or*/
    0,                                              /*nb_floor_divide*/
    0,                                              /*nb_true_divide*/
    0,                                              /*nb_inplace_floor_divide*/
    0,                                              /*nb_inplace_true_divide*/
    0,                                              /* nb_index */
};

PyTypeObject TrigXnoiseType = {
    PyObject_HEAD_INIT(NULL)
    0,                                              /*ob_size*/
    "_pyo.TrigXnoise_base",                                   /*tp_name*/
    sizeof(TrigXnoise),                                 /*tp_basicsize*/
    0,                                              /*tp_itemsize*/
    (destructor)TrigXnoise_dealloc,                     /*tp_dealloc*/
    0,                                              /*tp_print*/
    0,                                              /*tp_getattr*/
    0,                                              /*tp_setattr*/
    0,                                              /*tp_compare*/
    0,                                              /*tp_repr*/
    &TrigXnoise_as_number,                              /*tp_as_number*/
    0,                                              /*tp_as_sequence*/
    0,                                              /*tp_as_mapping*/
    0,                                              /*tp_hash */
    0,                                              /*tp_call*/
    0,                                              /*tp_str*/
    0,                                              /*tp_getattro*/
    0,                                              /*tp_setattro*/
    0,                                              /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "TrigXnoise objects. Periodically generates a new random value.",           /* tp_doc */
    (traverseproc)TrigXnoise_traverse,                  /* tp_traverse */
    (inquiry)TrigXnoise_clear,                          /* tp_clear */
    0,                                              /* tp_richcompare */
    0,                                              /* tp_weaklistoffset */
    0,                                              /* tp_iter */
    0,                                              /* tp_iternext */
    TrigXnoise_methods,                                 /* tp_methods */
    TrigXnoise_members,                                 /* tp_members */
    0,                                              /* tp_getset */
    0,                                              /* tp_base */
    0,                                              /* tp_dict */
    0,                                              /* tp_descr_get */
    0,                                              /* tp_descr_set */
    0,                                              /* tp_dictoffset */
    (initproc)TrigXnoise_init,                          /* tp_init */
    0,                                              /* tp_alloc */
    TrigXnoise_new,                                     /* tp_new */
};

/****************/
/**** TrigXnoiseMidi *****/
/****************/
typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *x1;
    PyObject *x2;
    Stream *x1_stream;
    Stream *x2_stream;
    float (*type_func_ptr)();
    int scale; // 0 = Midi, 1 = frequency, 2 = transpo
    int range_min;
    int range_max;
    int centralkey;
    float xx1;
    float xx2;
    int type;
    float value;
    float lastPoissonX1;
    int poisson_tab;
    float poisson_buffer[2000];
    float walkerValue;
    float loop_buffer[15];
    int loopChoice;
    int loopCountPlay;
    int loopTime;
    int loopCountRec;
    int loopLen;
    int loopStop;
    int modebuffer[4]; // need at least 2 slots for mul & add 
} TrigXnoiseMidi;

static float
TrigXnoiseMidi_convert(TrigXnoiseMidi *self) {
    int midival;
    float val;
    
    midival = (int)((self->value * (self->range_max-self->range_min)) + self->range_min);
    
    if (midival < 0)
        midival = 0;
    else if (midival > 127)
        midival = 127;
    
    if (self->scale == 0)
        val = (float)midival;
    else if (self->scale == 1)
        val = 8.175798 * powf(1.0594633, midival);
    else if (self->scale == 2)
        val = powf(1.0594633, midival - self->centralkey);
    else
        val = midival;

    return val;
}


// no parameter
static float
TrigXnoiseMidi_uniform(TrigXnoiseMidi *self) {
    return RANDOM_UNIFORM;    
}

static float
TrigXnoiseMidi_linear_min(TrigXnoiseMidi *self) {
    float a = RANDOM_UNIFORM;    
    float b = RANDOM_UNIFORM;
    if (a < b) return a;
    else return b;
}

static float
TrigXnoiseMidi_linear_max(TrigXnoiseMidi *self) {
    float a = RANDOM_UNIFORM;    
    float b = RANDOM_UNIFORM;
    if (a > b) return a;
    else return b;
}

static float
TrigXnoiseMidi_triangle(TrigXnoiseMidi *self) {
    float a = RANDOM_UNIFORM;    
    float b = RANDOM_UNIFORM;
    return ((a + b) * 0.5);
}

// x1 = slope
static float
TrigXnoiseMidi_expon_min(TrigXnoiseMidi *self) {
    if (self->xx1 <= 0.0) self->xx1 = 0.00001;
    float val = -logf(RANDOM_UNIFORM) / self->xx1;    
    if (val < 0.0) return 0.0;
    else if (val > 1.0) return 1.0;
    else return val;
}

static float
TrigXnoiseMidi_expon_max(TrigXnoiseMidi *self) {
    if (self->xx1 <= 0.0) self->xx1 = 0.00001;
    float val = 1.0 - (-logf(RANDOM_UNIFORM) / self->xx1);    
    if (val < 0.0) return 0.0;
    else if (val > 1.0) return 1.0;
    else return val;
}

// x1 = bandwidth
static float
TrigXnoiseMidi_biexpon(TrigXnoiseMidi *self) {
    float polar, val;
    if (self->xx1 <= 0.0) self->xx1 = 0.00001;
    float sum = RANDOM_UNIFORM * 2.0;
    
    if (sum > 1.0) {
        polar = -1;
        sum = 2.0 - sum;
    }
    else
        polar = 1;
    
    val = 0.5 * (polar * logf(sum) / self->xx1) + 0.5;
    
    if (val < 0.0) return 0.0;
    else if (val > 1.0) return 1.0;
    else return val;
}

static float
TrigXnoiseMidi_cauchy(TrigXnoiseMidi *self) {
    float rnd, val, dir;
    do {
        rnd = RANDOM_UNIFORM;
    }
    while (rnd == 0.5);
    
    if (rand() < (RAND_MAX / 2))
        dir = -1;
    else
        dir = 1;
    
    val = 0.5 * (tanf(rnd) * self->xx1 * dir) + 0.5;
    
    if (val < 0.0) return 0.0;
    else if (val > 1.0) return 1.0;
    else return val;
}

// x1 = locator, x2 = shape
static float
TrigXnoiseMidi_weibull(TrigXnoiseMidi *self) {
    float rnd, val;
    if (self->xx2 <= 0.0) self->xx2 = 0.00001;
    
    rnd = 1.0 / (1.0 - RANDOM_UNIFORM);
    val = self->xx1 * powf(logf(rnd), (1.0 / self->xx2));
    
    if (val < 0.0) return 0.0;
    else if (val > 1.0) return 1.0;
    else return val;
}

// x1 = locator, x2 = bandwidth
static float
TrigXnoiseMidi_gaussian(TrigXnoiseMidi *self) {
    float rnd, val;
    
    rnd = (RANDOM_UNIFORM + RANDOM_UNIFORM + RANDOM_UNIFORM + RANDOM_UNIFORM + RANDOM_UNIFORM + RANDOM_UNIFORM);
    val = (self->xx2 * (rnd - 3.0) * 0.33 + self->xx1);
    
    if (val < 0.0) return 0.0;
    else if (val > 1.0) return 1.0;
    else return val;
}

// x1 = gravity center, x2 = compress/expand
static float
TrigXnoiseMidi_poisson(TrigXnoiseMidi *self) {
    int i, j, factorial;
    long tot;
    float val;
    if (self->xx1 < 0.1) self->xx1 = 0.1;
    if (self->xx2 < 0.1) self->xx2 = 0.1;
    
    if (self->xx1 != self->lastPoissonX1) {
        self->lastPoissonX1 = self->xx1;
        self->poisson_tab = 0;
        factorial = 1;
        for (i=1; i<12; i++) {
            factorial *= i;
            tot = (long)(1000.0 * (powf(2.7182818, -self->xx1) * powf(self->xx1, i) / factorial));
            for (j=0; j<tot; j++) {
                self->poisson_buffer[self->poisson_tab] = i;
                self->poisson_tab++;
            }
        }
    }
    val = self->poisson_buffer[rand() % self->poisson_tab] / 12.0 * self->xx2;
    
    if (val < 0.0) return 0.0;
    else if (val > 1.0) return 1.0;
    else return val;
}

// x1 = max value, x2 = max step
static float
TrigXnoiseMidi_walker(TrigXnoiseMidi *self) {
    int modulo, dir;
    
    if (self->xx2 < 0.002) self->xx2 = 0.002;
    
    modulo = (int)(self->xx2 * 1000.0);
    dir = rand() % 2;
    
    if (dir == 0)
        self->walkerValue = self->walkerValue + (((rand() % modulo) - (modulo / 2)) * 0.001);
    else
        self->walkerValue = self->walkerValue - (((rand() % modulo) - (modulo / 2)) * 0.001);
    
    if (self->walkerValue > self->xx1)
        self->walkerValue = self->xx1;
    if (self->walkerValue < 0.0)
        self->walkerValue = 0.0;
    
    return self->walkerValue;
}

// x1 = max value, x2 = max step
static float
TrigXnoiseMidi_loopseg(TrigXnoiseMidi *self) {
    int modulo, dir;
    
    if (self->loopChoice == 0) {
        
        self->loopCountPlay = self->loopTime = 0;
        
        if (self->xx2 < 0.002) self->xx2 = 0.002;
        
        modulo = (int)(self->xx2 * 1000.0);
        dir = rand() % 2;
        
        if (dir == 0)
            self->walkerValue = self->walkerValue + (((rand() % modulo) - (modulo / 2)) * 0.001);
        else
            self->walkerValue = self->walkerValue - (((rand() % modulo) - (modulo / 2)) * 0.001);
        
        if (self->walkerValue > self->xx1)
            self->walkerValue = self->xx1;
        if (self->walkerValue < 0.0)
            self->walkerValue = 0.0;
        
        self->loop_buffer[self->loopCountRec++] = self->walkerValue;
        
        if (self->loopCountRec < self->loopLen)
            self->loopChoice = 0;
        else {
            self->loopChoice = 1;
            self->loopStop = (rand() % 4) + 1;
        }
    }
    else {
        self->loopCountRec = 0;
        
        self->walkerValue = self->loop_buffer[self->loopCountPlay++];
        
        if (self->loopCountPlay < self->loopLen)
            self->loopChoice = 1;
        else {
            self->loopCountPlay = 0;
            self->loopTime++;
        }
        
        if (self->loopTime == self->loopStop) {
            self->loopChoice = 0;
            self->loopLen = (rand() % 10) + 3;
        }
    }
    
    return self->walkerValue;
}

static void
TrigXnoiseMidi_generate_ii(TrigXnoiseMidi *self) {
    int i;
    float *in = Stream_getData((Stream *)self->input_stream);
    self->xx1 = PyFloat_AS_DOUBLE(self->x1);
    self->xx2 = PyFloat_AS_DOUBLE(self->x2);
    
    for (i=0; i<self->bufsize; i++) {
        if (in[i] == 1) {
            self->value = (*self->type_func_ptr)(self);
            self->value = TrigXnoiseMidi_convert(self);
        }    
        self->data[i] = self->value;
    }
}

static void
TrigXnoiseMidi_generate_ai(TrigXnoiseMidi *self) {
    int i;
    float *in = Stream_getData((Stream *)self->input_stream);
    float *x1 = Stream_getData((Stream *)self->x1_stream);
    self->xx2 = PyFloat_AS_DOUBLE(self->x2);
    
    for (i=0; i<self->bufsize; i++) {
        if (in[i] == 1) {
            self->xx1 = x1[i];
            self->value = (*self->type_func_ptr)(self);
            self->value = TrigXnoiseMidi_convert(self);
        }
        self->data[i] = self->value;
    }
}

static void
TrigXnoiseMidi_generate_ia(TrigXnoiseMidi *self) {
    int i;
    float *in = Stream_getData((Stream *)self->input_stream);
    self->xx1 = PyFloat_AS_DOUBLE(self->x1);
    float *x2 = Stream_getData((Stream *)self->x2_stream);
    
    for (i=0; i<self->bufsize; i++) {
        if (in[i] == 1) {
            self->xx2 = x2[i];
            self->value = (*self->type_func_ptr)(self);
            self->value = TrigXnoiseMidi_convert(self);
        }
        self->data[i] = self->value;
    }
}

static void
TrigXnoiseMidi_generate_aa(TrigXnoiseMidi *self) {
    int i;
    float *in = Stream_getData((Stream *)self->input_stream);
    float *x1 = Stream_getData((Stream *)self->x1_stream);
    float *x2 = Stream_getData((Stream *)self->x2_stream);
    
    for (i=0; i<self->bufsize; i++) {
        if (in[i] == 1) {
            self->xx1 = x1[i];
            self->xx2 = x2[i];
            self->value = (*self->type_func_ptr)(self);
            self->value = TrigXnoiseMidi_convert(self);
        }
        self->data[i] = self->value;
    }
}

static void TrigXnoiseMidi_postprocessing_ii(TrigXnoiseMidi *self) { POST_PROCESSING_II };
static void TrigXnoiseMidi_postprocessing_ai(TrigXnoiseMidi *self) { POST_PROCESSING_AI };
static void TrigXnoiseMidi_postprocessing_ia(TrigXnoiseMidi *self) { POST_PROCESSING_IA };
static void TrigXnoiseMidi_postprocessing_aa(TrigXnoiseMidi *self) { POST_PROCESSING_AA };
static void TrigXnoiseMidi_postprocessing_ireva(TrigXnoiseMidi *self) { POST_PROCESSING_IREVA };
static void TrigXnoiseMidi_postprocessing_areva(TrigXnoiseMidi *self) { POST_PROCESSING_AREVA };
static void TrigXnoiseMidi_postprocessing_revai(TrigXnoiseMidi *self) { POST_PROCESSING_REVAI };
static void TrigXnoiseMidi_postprocessing_revaa(TrigXnoiseMidi *self) { POST_PROCESSING_REVAA };
static void TrigXnoiseMidi_postprocessing_revareva(TrigXnoiseMidi *self) { POST_PROCESSING_REVAREVA };

static void
TrigXnoiseMidi_setRandomType(TrigXnoiseMidi *self)
{
    
    switch (self->type) {            
        case 0:
            self->type_func_ptr = TrigXnoiseMidi_uniform;
            break;
        case 1:
            self->type_func_ptr = TrigXnoiseMidi_linear_min;
            break;
        case 2:
            self->type_func_ptr = TrigXnoiseMidi_linear_max;
            break;
        case 3:
            self->type_func_ptr = TrigXnoiseMidi_triangle;
            break;
        case 4:
            self->type_func_ptr = TrigXnoiseMidi_expon_min;
            break;
        case 5:
            self->type_func_ptr = TrigXnoiseMidi_expon_max;
            break;
        case 6:
            self->type_func_ptr = TrigXnoiseMidi_biexpon;
            break;
        case 7:
            self->type_func_ptr = TrigXnoiseMidi_cauchy;
            break;
        case 8:
            self->type_func_ptr = TrigXnoiseMidi_weibull;
            break;
        case 9:
            self->type_func_ptr = TrigXnoiseMidi_gaussian;
            break;
        case 10:
            self->type_func_ptr = TrigXnoiseMidi_poisson;
            break;
        case 11:
            self->type_func_ptr = TrigXnoiseMidi_walker;
            break;
        case 12:
            self->type_func_ptr = TrigXnoiseMidi_loopseg;
            break;
    }        
}

static void
TrigXnoiseMidi_setProcMode(TrigXnoiseMidi *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2] + self->modebuffer[3] * 10;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;
    
	switch (procmode) {
        case 0:    
            self->proc_func_ptr = TrigXnoiseMidi_generate_ii;
            break;
        case 1:    
            self->proc_func_ptr = TrigXnoiseMidi_generate_ai;
            break;
        case 10:    
            self->proc_func_ptr = TrigXnoiseMidi_generate_ia;
            break;
        case 11:    
            self->proc_func_ptr = TrigXnoiseMidi_generate_aa;
            break;
    } 
	switch (muladdmode) {
        case 0:        
            self->muladd_func_ptr = TrigXnoiseMidi_postprocessing_ii;
            break;
        case 1:    
            self->muladd_func_ptr = TrigXnoiseMidi_postprocessing_ai;
            break;
        case 2:    
            self->muladd_func_ptr = TrigXnoiseMidi_postprocessing_revai;
            break;
        case 10:        
            self->muladd_func_ptr = TrigXnoiseMidi_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = TrigXnoiseMidi_postprocessing_aa;
            break;
        case 12:    
            self->muladd_func_ptr = TrigXnoiseMidi_postprocessing_revaa;
            break;
        case 20:        
            self->muladd_func_ptr = TrigXnoiseMidi_postprocessing_ireva;
            break;
        case 21:    
            self->muladd_func_ptr = TrigXnoiseMidi_postprocessing_areva;
            break;
        case 22:    
            self->muladd_func_ptr = TrigXnoiseMidi_postprocessing_revareva;
            break;
    }  
}

static void
TrigXnoiseMidi_compute_next_data_frame(TrigXnoiseMidi *self)
{
    (*self->proc_func_ptr)(self); 
    (*self->muladd_func_ptr)(self);
    Stream_setData(self->stream, self->data);
}

static int
TrigXnoiseMidi_traverse(TrigXnoiseMidi *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    Py_VISIT(self->x1);    
    Py_VISIT(self->x1_stream);    
    Py_VISIT(self->x2);    
    Py_VISIT(self->x2_stream);    
    return 0;
}

static int 
TrigXnoiseMidi_clear(TrigXnoiseMidi *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    Py_CLEAR(self->x1);    
    Py_CLEAR(self->x1_stream);    
    Py_CLEAR(self->x2);    
    Py_CLEAR(self->x2_stream);    
    return 0;
}

static void
TrigXnoiseMidi_dealloc(TrigXnoiseMidi* self)
{
    free(self->data);
    TrigXnoiseMidi_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * TrigXnoiseMidi_deleteStream(TrigXnoiseMidi *self) { DELETE_STREAM };

static PyObject *
TrigXnoiseMidi_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    TrigXnoiseMidi *self;
    self = (TrigXnoiseMidi *)type->tp_alloc(type, 0);
    
    srand((unsigned)(time(0)));
    
    self->x1 = PyFloat_FromDouble(0.5);
    self->x2 = PyFloat_FromDouble(0.5);
    self->xx1 = self->xx2 = self->walkerValue = 0.5;
    self->value = 0.0;
    self->scale = 0;
    self->range_min = 0;
    self->range_max = 127;
    self->centralkey = 64;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;
	self->modebuffer[3] = 0;
    
    self->poisson_tab = 0;
    self->lastPoissonX1 = -99.0;
    for (i=0; i<2000; i++) {
        self->poisson_buffer[i] = 0.0;
    }
    for (i=0; i<15; i++) {
        self->loop_buffer[i] = 0.0;
    }
    self->loopChoice = self->loopCountPlay = self->loopTime = self->loopCountRec = self->loopStop = 0;    
    self->loopLen = (rand() % 10) + 3;
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, TrigXnoiseMidi_compute_next_data_frame);
    self->mode_func_ptr = TrigXnoiseMidi_setProcMode;
    return (PyObject *)self;
}

static int
TrigXnoiseMidi_init(TrigXnoiseMidi *self, PyObject *args, PyObject *kwds)
{
    PyObject *inputtmp, *input_streamtmp, *x1tmp=NULL, *x2tmp=NULL, *rangetmp=NULL, *multmp=NULL, *addtmp=NULL;
    
    static char *kwlist[] = {"input", "type", "x1", "x2", "scale", "range", "mul", "add", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|iOOiOOO", kwlist, &inputtmp, &self->type, &x1tmp, &x2tmp, &self->scale, &rangetmp, &multmp, &addtmp))
        return -1; 
    
    Py_XDECREF(self->input);
    self->input = inputtmp;
    input_streamtmp = PyObject_CallMethod((PyObject *)self->input, "_getStream", NULL);
    Py_INCREF(input_streamtmp);
    Py_XDECREF(self->input_stream);
    self->input_stream = (Stream *)input_streamtmp;
    
    if (x1tmp) {
        PyObject_CallMethod((PyObject *)self, "setX1", "O", x1tmp);
    }
    
    if (x2tmp) {
        PyObject_CallMethod((PyObject *)self, "setX2", "O", x2tmp);
    }

    if (rangetmp) {
        PyObject_CallMethod((PyObject *)self, "setRange", "O", rangetmp);
    }
    
    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }
    
    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }
    
    Py_INCREF(self->stream);
    PyObject_CallMethod(self->server, "addStream", "O", self->stream);
    
    TrigXnoiseMidi_setRandomType(self);
    
    (*self->mode_func_ptr)(self);
    
    TrigXnoiseMidi_compute_next_data_frame((TrigXnoiseMidi *)self);
    
    Py_INCREF(self);
    return 0;
}

static PyObject * TrigXnoiseMidi_getServer(TrigXnoiseMidi* self) { GET_SERVER };
static PyObject * TrigXnoiseMidi_getStream(TrigXnoiseMidi* self) { GET_STREAM };
static PyObject * TrigXnoiseMidi_setMul(TrigXnoiseMidi *self, PyObject *arg) { SET_MUL };	
static PyObject * TrigXnoiseMidi_setAdd(TrigXnoiseMidi *self, PyObject *arg) { SET_ADD };	
static PyObject * TrigXnoiseMidi_setSub(TrigXnoiseMidi *self, PyObject *arg) { SET_SUB };	
static PyObject * TrigXnoiseMidi_setDiv(TrigXnoiseMidi *self, PyObject *arg) { SET_DIV };	

static PyObject * TrigXnoiseMidi_play(TrigXnoiseMidi *self) { PLAY };
static PyObject * TrigXnoiseMidi_out(TrigXnoiseMidi *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * TrigXnoiseMidi_stop(TrigXnoiseMidi *self) { STOP };

static PyObject * TrigXnoiseMidi_multiply(TrigXnoiseMidi *self, PyObject *arg) { MULTIPLY };
static PyObject * TrigXnoiseMidi_inplace_multiply(TrigXnoiseMidi *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * TrigXnoiseMidi_add(TrigXnoiseMidi *self, PyObject *arg) { ADD };
static PyObject * TrigXnoiseMidi_inplace_add(TrigXnoiseMidi *self, PyObject *arg) { INPLACE_ADD };
static PyObject * TrigXnoiseMidi_sub(TrigXnoiseMidi *self, PyObject *arg) { SUB };
static PyObject * TrigXnoiseMidi_inplace_sub(TrigXnoiseMidi *self, PyObject *arg) { INPLACE_SUB };
static PyObject * TrigXnoiseMidi_div(TrigXnoiseMidi *self, PyObject *arg) { DIV };
static PyObject * TrigXnoiseMidi_inplace_div(TrigXnoiseMidi *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
TrigXnoiseMidi_setType(TrigXnoiseMidi *self, PyObject *arg)
{	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyInt_Check(arg);
	
	if (isNumber == 1) {
		self->type = PyInt_AsLong(arg);
        TrigXnoiseMidi_setRandomType(self);
	}
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyObject *
TrigXnoiseMidi_setScale(TrigXnoiseMidi *self, PyObject *arg)
{	
    int tmp;
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyInt_Check(arg);
	
	if (isNumber == 1) {
		tmp = PyInt_AsLong(arg);
        if (tmp >= 0 && tmp <= 2)
            self->scale = tmp;
        else
            printf("scale attribute must be an integer {0, 1, 2}\n");
	}
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyObject *
TrigXnoiseMidi_setRange(TrigXnoiseMidi *self, PyObject *args)
{	
	if (args == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isTuple = PyTuple_Check(args);
    
	if (isTuple == 1) {
        self->range_min = PyInt_AsLong(PyTuple_GET_ITEM(args, 0));
        self->range_max = PyInt_AsLong(PyTuple_GET_ITEM(args, 1));
        self->centralkey = (int)((self->range_max + self->range_min) / 2);
	}
    
    Py_INCREF(Py_None);
	return Py_None;
}	

static PyObject *
TrigXnoiseMidi_setX1(TrigXnoiseMidi *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);
	
	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->x1);
	if (isNumber == 1) {
		self->x1 = PyNumber_Float(tmp);
        self->modebuffer[2] = 0;
	}
	else {
		self->x1 = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->x1, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->x1_stream);
        self->x1_stream = (Stream *)streamtmp;
		self->modebuffer[2] = 1;
	}
    
    (*self->mode_func_ptr)(self);
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyObject *
TrigXnoiseMidi_setX2(TrigXnoiseMidi *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);
	
	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->x2);
	if (isNumber == 1) {
		self->x2 = PyNumber_Float(tmp);
        self->modebuffer[3] = 0;
	}
	else {
		self->x2 = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->x2, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->x2_stream);
        self->x2_stream = (Stream *)streamtmp;
		self->modebuffer[3] = 1;
	}
    
    (*self->mode_func_ptr)(self);
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyMemberDef TrigXnoiseMidi_members[] = {
    {"server", T_OBJECT_EX, offsetof(TrigXnoiseMidi, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(TrigXnoiseMidi, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(TrigXnoiseMidi, input), 0, "Trigger input."},
    {"x1", T_OBJECT_EX, offsetof(TrigXnoiseMidi, x1), 0, "first param."},
    {"x2", T_OBJECT_EX, offsetof(TrigXnoiseMidi, x2), 0, "second param."},
    {"mul", T_OBJECT_EX, offsetof(TrigXnoiseMidi, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(TrigXnoiseMidi, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef TrigXnoiseMidi_methods[] = {
    {"getServer", (PyCFunction)TrigXnoiseMidi_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)TrigXnoiseMidi_getStream, METH_NOARGS, "Returns stream object."},
    {"deleteStream", (PyCFunction)TrigXnoiseMidi_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
    {"play", (PyCFunction)TrigXnoiseMidi_play, METH_NOARGS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)TrigXnoiseMidi_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)TrigXnoiseMidi_stop, METH_NOARGS, "Stops computing."},
    {"setType", (PyCFunction)TrigXnoiseMidi_setType, METH_O, "Sets distribution type."},
    {"setScale", (PyCFunction)TrigXnoiseMidi_setScale, METH_O, "Sets output scale."},
    {"setRange", (PyCFunction)TrigXnoiseMidi_setRange, METH_VARARGS, "Sets range in midi notes (min, max)."},
    {"setX1", (PyCFunction)TrigXnoiseMidi_setX1, METH_O, "Sets first param."},
    {"setX2", (PyCFunction)TrigXnoiseMidi_setX2, METH_O, "Sets second param."},
    {"setMul", (PyCFunction)TrigXnoiseMidi_setMul, METH_O, "Sets oscillator mul factor."},
    {"setAdd", (PyCFunction)TrigXnoiseMidi_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)TrigXnoiseMidi_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)TrigXnoiseMidi_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods TrigXnoiseMidi_as_number = {
    (binaryfunc)TrigXnoiseMidi_add,                         /*nb_add*/
    (binaryfunc)TrigXnoiseMidi_sub,                         /*nb_subtract*/
    (binaryfunc)TrigXnoiseMidi_multiply,                    /*nb_multiply*/
    (binaryfunc)TrigXnoiseMidi_div,                                              /*nb_divide*/
    0,                                              /*nb_remainder*/
    0,                                              /*nb_divmod*/
    0,                                              /*nb_power*/
    0,                                              /*nb_neg*/
    0,                                              /*nb_pos*/
    0,                                              /*(unaryfunc)array_abs,*/
    0,                                              /*nb_nonzero*/
    0,                                              /*nb_invert*/
    0,                                              /*nb_lshift*/
    0,                                              /*nb_rshift*/
    0,                                              /*nb_and*/
    0,                                              /*nb_xor*/
    0,                                              /*nb_or*/
    0,                                              /*nb_coerce*/
    0,                                              /*nb_int*/
    0,                                              /*nb_long*/
    0,                                              /*nb_float*/
    0,                                              /*nb_oct*/
    0,                                              /*nb_hex*/
    (binaryfunc)TrigXnoiseMidi_inplace_add,                 /*inplace_add*/
    (binaryfunc)TrigXnoiseMidi_inplace_sub,                 /*inplace_subtract*/
    (binaryfunc)TrigXnoiseMidi_inplace_multiply,            /*inplace_multiply*/
    (binaryfunc)TrigXnoiseMidi_inplace_div,                                              /*inplace_divide*/
    0,                                              /*inplace_remainder*/
    0,                                              /*inplace_power*/
    0,                                              /*inplace_lshift*/
    0,                                              /*inplace_rshift*/
    0,                                              /*inplace_and*/
    0,                                              /*inplace_xor*/
    0,                                              /*inplace_or*/
    0,                                              /*nb_floor_divide*/
    0,                                              /*nb_true_divide*/
    0,                                              /*nb_inplace_floor_divide*/
    0,                                              /*nb_inplace_true_divide*/
    0,                                              /* nb_index */
};

PyTypeObject TrigXnoiseMidiType = {
    PyObject_HEAD_INIT(NULL)
    0,                                              /*ob_size*/
    "_pyo.TrigXnoiseMidi_base",                                   /*tp_name*/
    sizeof(TrigXnoiseMidi),                                 /*tp_basicsize*/
    0,                                              /*tp_itemsize*/
    (destructor)TrigXnoiseMidi_dealloc,                     /*tp_dealloc*/
    0,                                              /*tp_print*/
    0,                                              /*tp_getattr*/
    0,                                              /*tp_setattr*/
    0,                                              /*tp_compare*/
    0,                                              /*tp_repr*/
    &TrigXnoiseMidi_as_number,                              /*tp_as_number*/
    0,                                              /*tp_as_sequence*/
    0,                                              /*tp_as_mapping*/
    0,                                              /*tp_hash */
    0,                                              /*tp_call*/
    0,                                              /*tp_str*/
    0,                                              /*tp_getattro*/
    0,                                              /*tp_setattro*/
    0,                                              /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "TrigXnoiseMidi objects. Periodically generates a new random value.",           /* tp_doc */
    (traverseproc)TrigXnoiseMidi_traverse,                  /* tp_traverse */
    (inquiry)TrigXnoiseMidi_clear,                          /* tp_clear */
    0,                                              /* tp_richcompare */
    0,                                              /* tp_weaklistoffset */
    0,                                              /* tp_iter */
    0,                                              /* tp_iternext */
    TrigXnoiseMidi_methods,                                 /* tp_methods */
    TrigXnoiseMidi_members,                                 /* tp_members */
    0,                                              /* tp_getset */
    0,                                              /* tp_base */
    0,                                              /* tp_dict */
    0,                                              /* tp_descr_get */
    0,                                              /* tp_descr_set */
    0,                                              /* tp_dictoffset */
    (initproc)TrigXnoiseMidi_init,                          /* tp_init */
    0,                                              /* tp_alloc */
    TrigXnoiseMidi_new,                                     /* tp_new */
};

/***************************************************/
/******* Counter ***********/
/***************************************************/
typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    long tmp;
    long min;
    long max;
    int dir;
    int direction;
    float value;
    int modebuffer[2]; // need at least 2 slots for mul & add 
} Counter;

static void
Counter_generates(Counter *self) {
    int i;
    float *in = Stream_getData((Stream *)self->input_stream);
    
    for (i=0; i<self->bufsize; i++) {
        if (in[i] == 1) {
            if (self->dir == 0) {
                self->value = (float)self->tmp;
                self->tmp++;
                if (self->tmp > self->max)
                    self->tmp = self->min;
            }    
            else if (self->dir == 1) {
                self->value = (float)self->tmp;
                self->tmp--;
                if (self->tmp < self->min)
                    self->tmp = self->max;
            }    
            else if (self->dir == 2) {
                self->value = (float)self->tmp;
                self->tmp = self->tmp + self->direction;
                if (self->tmp >= self->max) {
                    self->direction = -1;
                    self->tmp--;
                }    
                else if (self->tmp <= self->min) {
                    self->direction = 1;
                    self->tmp++;
                }    
            }
        }
        self->data[i] = self->value;
    }
}

static void Counter_postprocessing_ii(Counter *self) { POST_PROCESSING_II };
static void Counter_postprocessing_ai(Counter *self) { POST_PROCESSING_AI };
static void Counter_postprocessing_ia(Counter *self) { POST_PROCESSING_IA };
static void Counter_postprocessing_aa(Counter *self) { POST_PROCESSING_AA };
static void Counter_postprocessing_ireva(Counter *self) { POST_PROCESSING_IREVA };
static void Counter_postprocessing_areva(Counter *self) { POST_PROCESSING_AREVA };
static void Counter_postprocessing_revai(Counter *self) { POST_PROCESSING_REVAI };
static void Counter_postprocessing_revaa(Counter *self) { POST_PROCESSING_REVAA };
static void Counter_postprocessing_revareva(Counter *self) { POST_PROCESSING_REVAREVA };

static void
Counter_setProcMode(Counter *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;
    
    self->proc_func_ptr = Counter_generates;

	switch (muladdmode) {
        case 0:        
            self->muladd_func_ptr = Counter_postprocessing_ii;
            break;
        case 1:    
            self->muladd_func_ptr = Counter_postprocessing_ai;
            break;
        case 2:    
            self->muladd_func_ptr = Counter_postprocessing_revai;
            break;
        case 10:        
            self->muladd_func_ptr = Counter_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = Counter_postprocessing_aa;
            break;
        case 12:    
            self->muladd_func_ptr = Counter_postprocessing_revaa;
            break;
        case 20:        
            self->muladd_func_ptr = Counter_postprocessing_ireva;
            break;
        case 21:    
            self->muladd_func_ptr = Counter_postprocessing_areva;
            break;
        case 22:    
            self->muladd_func_ptr = Counter_postprocessing_revareva;
            break;
    }  
}

static void
Counter_compute_next_data_frame(Counter *self)
{
    (*self->proc_func_ptr)(self); 
    (*self->muladd_func_ptr)(self);
    Stream_setData(self->stream, self->data);
}

static int
Counter_traverse(Counter *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    return 0;
}

static int 
Counter_clear(Counter *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    return 0;
}

static void
Counter_dealloc(Counter* self)
{
    free(self->data);
    Counter_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * Counter_deleteStream(Counter *self) { DELETE_STREAM };

static PyObject *
Counter_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    Counter *self;
    self = (Counter *)type->tp_alloc(type, 0);
    
    self->min = 0;
    self->max = 100;
    self->dir = 0;
    self->direction = 1;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Counter_compute_next_data_frame);
    self->mode_func_ptr = Counter_setProcMode;
    return (PyObject *)self;
}

static int
Counter_init(Counter *self, PyObject *args, PyObject *kwds)
{
    PyObject *inputtmp, *input_streamtmp, *multmp=NULL, *addtmp=NULL;
    
    static char *kwlist[] = {"input", "min", "max", "dir", "mul", "add", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|lliOO", kwlist, &inputtmp, &self->min, &self->max, &self->dir, &multmp, &addtmp))
        return -1; 
    
    Py_XDECREF(self->input);
    self->input = inputtmp;
    input_streamtmp = PyObject_CallMethod((PyObject *)self->input, "_getStream", NULL);
    Py_INCREF(input_streamtmp);
    Py_XDECREF(self->input_stream);
    self->input_stream = (Stream *)input_streamtmp;

    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }
    
    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }
    
    Py_INCREF(self->stream);
    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    if (self->dir == 0 || self->dir == 2)
        self->tmp = self->min;
    else
        self->tmp = self->max;
    
    (*self->mode_func_ptr)(self);
    
    Counter_compute_next_data_frame((Counter *)self);
    
    Py_INCREF(self);
    return 0;
}

static PyObject * Counter_getServer(Counter* self) { GET_SERVER };
static PyObject * Counter_getStream(Counter* self) { GET_STREAM };
static PyObject * Counter_setMul(Counter *self, PyObject *arg) { SET_MUL };	
static PyObject * Counter_setAdd(Counter *self, PyObject *arg) { SET_ADD };	
static PyObject * Counter_setSub(Counter *self, PyObject *arg) { SET_SUB };	
static PyObject * Counter_setDiv(Counter *self, PyObject *arg) { SET_DIV };	

static PyObject * Counter_play(Counter *self) { PLAY };
static PyObject * Counter_stop(Counter *self) { STOP };

static PyObject * Counter_multiply(Counter *self, PyObject *arg) { MULTIPLY };
static PyObject * Counter_inplace_multiply(Counter *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Counter_add(Counter *self, PyObject *arg) { ADD };
static PyObject * Counter_inplace_add(Counter *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Counter_sub(Counter *self, PyObject *arg) { SUB };
static PyObject * Counter_inplace_sub(Counter *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Counter_div(Counter *self, PyObject *arg) { DIV };
static PyObject * Counter_inplace_div(Counter *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
Counter_setMin(Counter *self, PyObject *arg)
{	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	if (PyLong_Check(arg) || PyInt_Check(arg)) {	
		self->min = PyLong_AsLong(arg);
	}

	Py_INCREF(Py_None);
	return Py_None;
}	

static PyObject *
Counter_setMax(Counter *self, PyObject *arg)
{	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	if (PyLong_Check(arg) || PyInt_Check(arg)) {	
		self->max = PyLong_AsLong(arg);
	}
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyObject *
Counter_setDir(Counter *self, PyObject *arg)
{	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	if (PyInt_Check(arg)) {	
		self->dir = PyInt_AsLong(arg);
	}
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyMemberDef Counter_members[] = {
{"server", T_OBJECT_EX, offsetof(Counter, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(Counter, stream), 0, "Stream object."},
{"input", T_OBJECT_EX, offsetof(Counter, input), 0, "Input sound object."},
{"mul", T_OBJECT_EX, offsetof(Counter, mul), 0, "Mul factor."},
{"add", T_OBJECT_EX, offsetof(Counter, add), 0, "Add factor."},
{NULL}  /* Sentinel */
};

static PyMethodDef Counter_methods[] = {
{"getServer", (PyCFunction)Counter_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)Counter_getStream, METH_NOARGS, "Returns stream object."},
{"deleteStream", (PyCFunction)Counter_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
{"play", (PyCFunction)Counter_play, METH_NOARGS, "Starts computing without sending sound to soundcard."},
{"stop", (PyCFunction)Counter_stop, METH_NOARGS, "Stops computing."},
{"setMin", (PyCFunction)Counter_setMin, METH_O, "Sets minimum value."},
{"setMax", (PyCFunction)Counter_setMax, METH_O, "Sets maximum value."},
{"setDir", (PyCFunction)Counter_setDir, METH_O, "Sets direction. 0 = forward, 1 = backward, 2 = back and forth"},
{"setMul", (PyCFunction)Counter_setMul, METH_O, "Sets oscillator mul factor."},
{"setAdd", (PyCFunction)Counter_setAdd, METH_O, "Sets oscillator add factor."},
{"setSub", (PyCFunction)Counter_setSub, METH_O, "Sets inverse add factor."},
{"setDiv", (PyCFunction)Counter_setDiv, METH_O, "Sets inverse mul factor."},
{NULL}  /* Sentinel */
};

static PyNumberMethods Counter_as_number = {
(binaryfunc)Counter_add,                         /*nb_add*/
(binaryfunc)Counter_sub,                         /*nb_subtract*/
(binaryfunc)Counter_multiply,                    /*nb_multiply*/
(binaryfunc)Counter_div,                                              /*nb_divide*/
0,                                              /*nb_remainder*/
0,                                              /*nb_divmod*/
0,                                              /*nb_power*/
0,                                              /*nb_neg*/
0,                                              /*nb_pos*/
0,                                              /*(unaryfunc)array_abs,*/
0,                                              /*nb_nonzero*/
0,                                              /*nb_invert*/
0,                                              /*nb_lshift*/
0,                                              /*nb_rshift*/
0,                                              /*nb_and*/
0,                                              /*nb_xor*/
0,                                              /*nb_or*/
0,                                              /*nb_coerce*/
0,                                              /*nb_int*/
0,                                              /*nb_long*/
0,                                              /*nb_float*/
0,                                              /*nb_oct*/
0,                                              /*nb_hex*/
(binaryfunc)Counter_inplace_add,                 /*inplace_add*/
(binaryfunc)Counter_inplace_sub,                 /*inplace_subtract*/
(binaryfunc)Counter_inplace_multiply,            /*inplace_multiply*/
(binaryfunc)Counter_inplace_div,                                              /*inplace_divide*/
0,                                              /*inplace_remainder*/
0,                                              /*inplace_power*/
0,                                              /*inplace_lshift*/
0,                                              /*inplace_rshift*/
0,                                              /*inplace_and*/
0,                                              /*inplace_xor*/
0,                                              /*inplace_or*/
0,                                              /*nb_floor_divide*/
0,                                              /*nb_true_divide*/
0,                                              /*nb_inplace_floor_divide*/
0,                                              /*nb_inplace_true_divide*/
0,                                              /* nb_index */
};

PyTypeObject CounterType = {
PyObject_HEAD_INIT(NULL)
0,                                              /*ob_size*/
"_pyo.Counter_base",                                   /*tp_name*/
sizeof(Counter),                                 /*tp_basicsize*/
0,                                              /*tp_itemsize*/
(destructor)Counter_dealloc,                     /*tp_dealloc*/
0,                                              /*tp_print*/
0,                                              /*tp_getattr*/
0,                                              /*tp_setattr*/
0,                                              /*tp_compare*/
0,                                              /*tp_repr*/
&Counter_as_number,                              /*tp_as_number*/
0,                                              /*tp_as_sequence*/
0,                                              /*tp_as_mapping*/
0,                                              /*tp_hash */
0,                                              /*tp_call*/
0,                                              /*tp_str*/
0,                                              /*tp_getattro*/
0,                                              /*tp_setattro*/
0,                                              /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
"Counter objects. Integer incrementor.",           /* tp_doc */
(traverseproc)Counter_traverse,                  /* tp_traverse */
(inquiry)Counter_clear,                          /* tp_clear */
0,                                              /* tp_richcompare */
0,                                              /* tp_weaklistoffset */
0,                                              /* tp_iter */
0,                                              /* tp_iternext */
Counter_methods,                                 /* tp_methods */
Counter_members,                                 /* tp_members */
0,                                              /* tp_getset */
0,                                              /* tp_base */
0,                                              /* tp_dict */
0,                                              /* tp_descr_get */
0,                                              /* tp_descr_set */
0,                                              /* tp_dictoffset */
(initproc)Counter_init,                          /* tp_init */
0,                                              /* tp_alloc */
Counter_new,                                     /* tp_new */
};

/***************************************************/
/******* Thresh ***********/
/***************************************************/
typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *threshold;
    Stream *threshold_stream;
    int dir;
    int ready;
    int modebuffer[1];
} Thresh;

static void
Thresh_generates_i(Thresh *self) {
    int i;
    float *in = Stream_getData((Stream *)self->input_stream);
    float thresh = PyFloat_AS_DOUBLE(self->threshold);

    switch (self->dir) {
        case 0:
            for (i=0; i<self->bufsize; i++) {
                self->data[i] = 0.0;
                if (in[i] > thresh && self->ready == 1) {
                    self->data[i] = 1.0;
                    self->ready = 0;
                }
                else if (in[i] <= thresh && self->ready == 0)
                    self->ready = 1;
            } 
            break;
        case 1:
            for (i=0; i<self->bufsize; i++) {
                self->data[i] = 0.0;
                if (in[i] < thresh && self->ready == 1) {
                    self->data[i] = 1.0;
                    self->ready = 0;
                }
                else if (in[i] >= thresh && self->ready == 0)
                    self->ready = 1;
            } 
            break;
        case 2:
            for (i=0; i<self->bufsize; i++) {
                self->data[i] = 0.0;
                if (in[i] > thresh && self->ready == 1) {
                    self->data[i] = 1.0;
                    self->ready = 0;
                }
                else if (in[i] <= thresh && self->ready == 0) {
                    self->data[i] = 1.0;
                    self->ready = 1;
                }    
            } 
            break;
    }
}

static void
Thresh_generates_a(Thresh *self) {
    int i;
    float *in = Stream_getData((Stream *)self->input_stream);
    float *thresh = Stream_getData((Stream *)self->threshold_stream);
    
    switch (self->dir) {
        case 0:
            for (i=0; i<self->bufsize; i++) {
                self->data[i] = 0.0;
                if (in[i] > thresh[i] && self->ready == 1) {
                    self->data[i] = 1.0;
                    self->ready = 0;
                }
                else if (in[i] <= thresh[i] && self->ready == 0)
                    self->ready = 1;
            } 
            break;
        case 1:
            for (i=0; i<self->bufsize; i++) {
                self->data[i] = 0.0;
                if (in[i] < thresh[i] && self->ready == 1) {
                    self->data[i] = 1.0;
                    self->ready = 0;
                }
                else if (in[i] >= thresh[i] && self->ready == 0)
                    self->ready = 1;
            } 
            break;
        case 2:
            for (i=0; i<self->bufsize; i++) {
                self->data[i] = 0.0;
                if (in[i] > thresh[i] && self->ready == 1) {
                    self->data[i] = 1.0;
                    self->ready = 0;
                }
                else if (in[i] <= thresh[i] && self->ready == 0)
                    self->data[i] = 1.0;
                self->ready = 1;
            } 
            break;
    }
}

static void
Thresh_setProcMode(Thresh *self)
{    
    int procmode = self->modebuffer[0];
    switch (procmode) {
        case 0:        
            self->proc_func_ptr = Thresh_generates_i;
            break;
        case 1:    
            self->proc_func_ptr = Thresh_generates_a;
            break;
    }
}

static void
Thresh_compute_next_data_frame(Thresh *self)
{
    (*self->proc_func_ptr)(self); 
    Stream_setData(self->stream, self->data);
}

static int
Thresh_traverse(Thresh *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    Py_VISIT(self->threshold);
    Py_VISIT(self->threshold_stream);
    return 0;
}

static int 
Thresh_clear(Thresh *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    Py_CLEAR(self->threshold);
    Py_CLEAR(self->threshold_stream);
    return 0;
}

static void
Thresh_dealloc(Thresh* self)
{
    free(self->data);
    Thresh_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * Thresh_deleteStream(Thresh *self) { DELETE_STREAM };

static PyObject *
Thresh_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    Thresh *self;
    self = (Thresh *)type->tp_alloc(type, 0);
    
    self->threshold = PyFloat_FromDouble(0.);
    self->dir = 0;
    self->ready = 1;
    self->modebuffer[0] = 0;
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Thresh_compute_next_data_frame);
    self->mode_func_ptr = Thresh_setProcMode;
    return (PyObject *)self;
}

static int
Thresh_init(Thresh *self, PyObject *args, PyObject *kwds)
{
    PyObject *inputtmp, *input_streamtmp, *thresholdtmp;
    
    static char *kwlist[] = {"input", "threshold", "dir", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|Oi", kwlist, &inputtmp, &thresholdtmp, &self->dir))
        return -1; 
    
    Py_XDECREF(self->input);
    self->input = inputtmp;
    input_streamtmp = PyObject_CallMethod((PyObject *)self->input, "_getStream", NULL);
    Py_INCREF(input_streamtmp);
    Py_XDECREF(self->input_stream);
    self->input_stream = (Stream *)input_streamtmp;
    
    if (thresholdtmp) {
        PyObject_CallMethod((PyObject *)self, "setThreshold", "O", thresholdtmp);
    }
   
    Py_INCREF(self->stream);
    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    (*self->mode_func_ptr)(self);
    
    Thresh_compute_next_data_frame((Thresh *)self);
    
    Py_INCREF(self);
    return 0;
}

static PyObject * Thresh_getServer(Thresh* self) { GET_SERVER };
static PyObject * Thresh_getStream(Thresh* self) { GET_STREAM };

static PyObject * Thresh_play(Thresh *self) { PLAY };
static PyObject * Thresh_stop(Thresh *self) { STOP };

static PyObject *
Thresh_setThreshold(Thresh *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);
	
	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->threshold);
	if (isNumber == 1) {
		self->threshold = PyNumber_Float(tmp);
        self->modebuffer[0] = 0;
	}
	else {
		self->threshold = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->threshold, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->threshold_stream);
        self->threshold_stream = (Stream *)streamtmp;
		self->modebuffer[0] = 1;
	}
    
    (*self->mode_func_ptr)(self);
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyObject *
Thresh_setDir(Thresh *self, PyObject *arg)
{	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	if (PyInt_Check(arg)) {	
		self->dir = PyInt_AsLong(arg);
	}
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyMemberDef Thresh_members[] = {
{"server", T_OBJECT_EX, offsetof(Thresh, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(Thresh, stream), 0, "Stream object."},
{"input", T_OBJECT_EX, offsetof(Thresh, input), 0, "Input sound object."},
{"threshold", T_OBJECT_EX, offsetof(Thresh, threshold), 0, "Threshold object."},
{NULL}  /* Sentinel */
};

static PyMethodDef Thresh_methods[] = {
{"getServer", (PyCFunction)Thresh_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)Thresh_getStream, METH_NOARGS, "Returns stream object."},
{"deleteStream", (PyCFunction)Thresh_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
{"play", (PyCFunction)Thresh_play, METH_NOARGS, "Starts computing without sending sound to soundcard."},
{"stop", (PyCFunction)Thresh_stop, METH_NOARGS, "Stops computing."},
{"setThreshold", (PyCFunction)Thresh_setThreshold, METH_O, "Sets threshold value."},
{"setDir", (PyCFunction)Thresh_setDir, METH_O, "Sets direction. 0 = upward, 1 = downward, 2 = up and down"},
{NULL}  /* Sentinel */
};

PyTypeObject ThreshType = {
PyObject_HEAD_INIT(NULL)
0,                                              /*ob_size*/
"_pyo.Thresh_base",                                   /*tp_name*/
sizeof(Thresh),                                 /*tp_basicsize*/
0,                                              /*tp_itemsize*/
(destructor)Thresh_dealloc,                     /*tp_dealloc*/
0,                                              /*tp_print*/
0,                                              /*tp_getattr*/
0,                                              /*tp_setattr*/
0,                                              /*tp_compare*/
0,                                              /*tp_repr*/
0,                              /*tp_as_number*/
0,                                              /*tp_as_sequence*/
0,                                              /*tp_as_mapping*/
0,                                              /*tp_hash */
0,                                              /*tp_call*/
0,                                              /*tp_str*/
0,                                              /*tp_getattro*/
0,                                              /*tp_setattro*/
0,                                              /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
"Thresh objects. Threshold detector.",           /* tp_doc */
(traverseproc)Thresh_traverse,                  /* tp_traverse */
(inquiry)Thresh_clear,                          /* tp_clear */
0,                                              /* tp_richcompare */
0,                                              /* tp_weaklistoffset */
0,                                              /* tp_iter */
0,                                              /* tp_iternext */
Thresh_methods,                                 /* tp_methods */
Thresh_members,                                 /* tp_members */
0,                                              /* tp_getset */
0,                                              /* tp_base */
0,                                              /* tp_dict */
0,                                              /* tp_descr_get */
0,                                              /* tp_descr_set */
0,                                              /* tp_dictoffset */
(initproc)Thresh_init,                          /* tp_init */
0,                                              /* tp_alloc */
Thresh_new,                                     /* tp_new */
};
