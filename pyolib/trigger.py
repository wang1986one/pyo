from _core import *

class TrigRand(PyoObject):
    """
    Pseudo-random number generator.
    
    TrigRand generates a pseudo-random number between `min` and `max` 
    values each time it receives a trigger in its `input` parameter. 
    The value is kept until the next trigger.
    
    Parent class: PyoObject

    Parameters:

    input : PyoObject
        Audio signal sending triggers.
    min : float or PyoObject, optional
        Minimum value for the random generation. Defaults to 0.
    max : float or PyoObject, optional
        Maximum value for the random generation. Defaults to 1.
    
    Methods:

    setInput(x, fadetime) : Replace the `input` attribute.
    setMin(x) : Replace the `min` attribute.
    setMax(x) : Replace the `max` attribute.

    Attributes:
    
    input : PyoObject. Audio trigger signal.
    min : float or PyoObject. Minimum value.
    max : float or PyoObject. Maximum value.

    Examples:
    
    >>> s = Server().boot()
    >>> s.start()
    >>> m = Metro(.125).play()
    >>> tr = TrigRand(m, 400, 600)
    >>> a = Sine(tr, mul=.5).out()
    
    """
    def __init__(self, input, min=0., max=1., mul=1, add=0):
        self._input = input
        self._min = min
        self._max = max
        self._mul = mul
        self._add = add
        self._in_fader = InputFader(input)
        in_fader, min, max, mul, add, lmax = convertArgsToLists(self._in_fader, min, max, mul, add)
        self._base_objs = [TrigRand_base(wrap(in_fader,i), wrap(min,i), wrap(max,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def setInput(self, x, fadetime=0.05):
        """
        Replace the `input` attribute.
        
        Parameters:

        x : PyoObject
            New signal to process.
        fadetime : float, optional
            Crossfade time between old and new input. Defaults to 0.05.

        """
        self._input = x
        self._in_fader.setInput(x, fadetime)
        
    def setMin(self, x):
        """Replace the `min` attribute.
        
        Parameters:

        x : float or PyoObject
            new `min` attribute.
        
        """
        self._min = x
        x, lmax = convertArgsToLists(x)
        [obj.setMin(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setMax(self, x):
        """
        Replace the `max` attribute.
        
        Parameters:

        x : float or PyoObject
            new `max` attribute.
        
        """
        self._max = x
        x, lmax = convertArgsToLists(x)
        [obj.setMax(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None):
        if map_list == None:
            map_list = [SLMap(0., 1., 'lin', 'min', self._min),
                        SLMap(1., 2., 'lin', 'max', self._max),
                        SLMapMul(self._mul)]
        win = Tk()    
        f = PyoObjectControl(win, self, map_list)
        if title == None: title = self.__class__.__name__
        win.title(title)

    #def demo():
    #    execfile(DEMOS_PATH + "/TrigRand_demo.py")
    #demo = Call_example(demo)

    def args():
        return('TrigRand(input, min=0., max=1., mul=1, add=0)')
    args = Print_args(args)

    @property
    def input(self): return self._input
    @input.setter
    def input(self, x): self.setInput(x)
    @property
    def min(self): return self._min
    @min.setter
    def min(self, x): self.setMin(x)
    @property
    def max(self): return self._max
    @max.setter
    def max(self, x): self.setMax(x)

class TrigChoice(PyoObject):
    """
    Random generator from user's defined values.
    
    TrigChoice chooses randomly a new value in list `choice` each 
    time it receives a trigger in its `input` parameter. The value 
    is kept until the next trigger.
    
    Parent class: PyoObject

    Parameters:

    input : PyoObject
        Audio signal sending triggers.
    choice : list of floats
        Possible values for the random generation.
    
    Methods:

    setInput(x, fadetime) : Replace the `input` attribute.
    setChoice(x) : Replace the `choice` attribute.

    Attributes:
    
    input : PyoObject. Audio trigger signal.
    choice : list of floats. Possible values.

    Examples:
    
    >>> s = Server().boot()
    >>> s.start()
    >>> m = Metro(.125).play()
    >>> TrigChoice(m, [300, 350, 400, 450, 500, 550])
    >>> a = Sine(tr, mul=.5).out()
    
    """
    def __init__(self, input, choice, mul=1, add=0):
        self._input = input
        self._choice = choice
        self._mul = mul
        self._add = add
        self._in_fader = InputFader(input)
        in_fader, mul, add, lmax = convertArgsToLists(self._in_fader, mul, add)
        self._base_objs = [TrigChoice_base(wrap(in_fader,i), choice, wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def setInput(self, x, fadetime=0.05):
        """
        Replace the `input` attribute.
        
        Parameters:

        x : PyoObject
            New signal to process.
        fadetime : float, optional
            Crossfade time between old and new input. Defaults to 0.05.

        """
        self._input = x
        self._in_fader.setInput(x, fadetime)
        
    def setChoice(self, x):
        """
        Replace the `choice` attribute.
        
        Parameters:

        x : list of floats
            new `choice` attribute.
        
        """
        self._choice = x
        [obj.setChoice(x) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None):
        print "There is no control for TrigChoice object."

    #def demo():
    #    execfile(DEMOS_PATH + "/TrigChoice_demo.py")
    #demo = Call_example(demo)

    def args():
        return('TrigChoice(input, choice, mul=1, add=0)')
    args = Print_args(args)

    @property
    def input(self): return self._input
    @input.setter
    def input(self, x): self.setInput(x)
    @property
    def choice(self): return self._choice
    @choice.setter
    def choice(self, x): self.setChoice(x)

class TrigFunc(PyoObject):
    """
    Python function callback.
    
    TrigFunc calls the function given at parameter `function` each 
    time it receives a trigger in its `input` parameter.
    
    Parent class: PyoObject

    Parameters:

    input : PyoObject
        Audio signal sending triggers.
    function : Python function
        Function to be called.
    
    Methods:

    setInput(x, fadetime) : Replace the `input` attribute.
    setFunction(x) : Replace the `function` attribute.

    Attributes:
    
    input : PyoObject. Audio trigger signal.
    function : Python function. Function to be called.

    Notes:

    The out() method is bypassed. TrigFunc's signal can not be sent 
    to audio outs.
    
    TrigFunc has no `mul` and `add` attributes.

    Examples:
    
    >>> s = Server().boot()
    >>> s.start()
    >>> c = 0
    >>> def count():
    ...     global c
    ...     c += 1
    ...     if c == 10:
    ...         m.stop()
    ...         a.stop()
    >>> m = Metro(.125).play()
    >>> tr = TrigRand(m, 400, 600)
    >>> tf = TrigFunc(m, count)
    >>> a = Sine(tr, mul=.5).out()

    """
    def __init__(self, input, function, mul=1, add=0):
        self._input = input
        self._function = function
        self._in_fader = InputFader(input)
        in_fader, function, lmax = convertArgsToLists(self._in_fader, function)
        self._base_objs = [TrigFunc_base(wrap(in_fader,i), wrap(function,i)) for i in range(lmax)]

    def out(self, chnl=0, inc=1):
        return self

    def setMul(self, x):
        pass
        
    def setAdd(self, x):
        pass    

    def setInput(self, x, fadetime=0.05):
        """
        Replace the `input` attribute.
        
        Parameters:

        x : PyoObject
            New signal to process.
        fadetime : float, optional
            Crossfade time between old and new input. Defaults to 0.05.

        """
        self._input = x
        self._in_fader.setInput(x, fadetime)
        
    def setFunction(self, x):
        """
        Replace the `function` attribute.
        
        Parameters:

        x : Python function
            new `function` attribute.
        
        """
        self._function = x
        x, lmax = convertArgsToLists(x)
        [obj.setFunction(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None):
        print "There is no control for TrigFunc object."

    #def demo():
    #    execfile(DEMOS_PATH + "/TrigFunction_demo.py")
    #demo = Call_example(demo)

    def args():
        return('TrigFunc(input, function)')
    args = Print_args(args)

    @property
    def input(self): return self._input
    @input.setter
    def input(self, x): self.setInput(x)
    @property
    def function(self): return self._function
    @function.setter
    def function(self, x): self.setFunction(x)   
     
class TrigEnv(PyoObject):
    """
    Envelope reader generator.
    
    TrigEnv starts reading an envelope in `dur` seconds each time it 
    receives a trigger in its `input` parameter.
    
    Parent class: PyoObject

    Parameters:

    input : PyoObject
        Audio signal sending triggers.
    table : PyoTableObject
        Table containing the envelope.
    dur : float or PyoObject, optional
        Duration in seconds of the envelope. Defaults to 1.
    
    Methods:

    setInput(x, fadetime) : Replace the `input` attribute.
    setTable(x) : Replace the `table` attribute.
    setDur(x) : Replace the `dur` attribute.

    Attributes:
    
    input : PyoObject. Audio trigger signal.
    table : PyoTableObject. Envelope table.
    dur : float or PyoObject. Duration in seconds.

    Examples:
    
    >>> s = Server().boot()
    >>> s.start()
    >>> env = HannTable()
    >>> m = Metro(.125).play()
    >>> tr = TrigRand(m, 400, 600)
    >>> te = TrigEnv(m, env, .125)
    >>> a = Sine(tr, mul=te).out()
    
    """
    def __init__(self, input, table, dur=1, mul=1, add=0):
        self._input = input
        self._table = table
        self._dur = dur
        self._mul = mul
        self._add = add
        self._in_fader = InputFader(input)
        in_fader, table, dur, mul, add, lmax = convertArgsToLists(self._in_fader, table, dur, mul, add)
        self._base_objs = [TrigEnv_base(wrap(in_fader,i), wrap(table,i), wrap(dur,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def setInput(self, x, fadetime=0.05):
        """
        Replace the `input` attribute.
        
        Parameters:

        x : PyoObject
            New signal to process.
        fadetime : float, optional
            Crossfade time between old and new input. Defaults to 0.05.

        """
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setTable(self, x):
        """Replace the `table` attribute.
        
        Parameters:

        x : PyoTableObject
            new `table` attribute.
        
        """
        self._table = x
        x, lmax = convertArgsToLists(x)
        [obj.setTable(wrap(x,i)) for i, obj in enumerate(self._base_objs)]
        
    def setDur(self, x):
        """Replace the `dur` attribute.
        
        Parameters:

        x : float or PyoObject
            new `dur` attribute.
        
        """
        self._dur = x
        x, lmax = convertArgsToLists(x)
        [obj.setDur(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None):
        if map_list == None:
            map_list = [SLMap(0.01, 10., 'lin', 'dur', self._dur),
                        SLMapMul(self._mul)]
        win = Tk()    
        f = PyoObjectControl(win, self, map_list)
        if title == None: title = self.__class__.__name__
        win.title(title)

    #def demo():
    #    execfile(DEMOS_PATH + "/TrigEnv_demo.py")
    #demo = Call_example(demo)

    def args():
        return('TrigEnv(input, table, dur=1, mul=1, add=0)')
    args = Print_args(args)

    @property
    def input(self): return self._input
    @input.setter
    def input(self, x): self.setInput(x)
    @property
    def table(self): return self._table
    @table.setter
    def table(self, x): self.setTable(x)
    @property
    def dur(self): return self._dur
    @dur.setter
    def dur(self, x): self.setDur(x)

class Counter(PyoObject):
    """
    Integer count generator.
    
    Counter keeps track of all triggers received, outputs the current 
    count constrained within `min` and `max` range, and can be set to 
    count up, down, or up-and-down.

    
    Parent class: PyoObject

    Parameters:

    input : PyoObject
        Audio signal sending triggers.
    min : int, optional
        Minimum value of the count, included in the count. Defaults to 0.
    max : int, optional
        Maximum value of the count. excluded of the count. 
        The counter will count up to max - 1. Defaults to 100.
    dir : int {0, 1, 2}, optional
        Direction of the count. Three possible values:
            0 : up
            1 : down
            2 : up-and-down
        Defaults to 0.    
    
    Methods:

    setInput(x, fadetime) : Replace the `input` attribute.
    setMin(x) : Replace the `min` attribute.
    setMax(x) : Replace the `max` attribute.
    setDir(x) : Replace the `dir` attribute.

    Attributes:
    
    input : PyoObject. Audio trigger signal.
    min : int. Minimum value.
    max : int. Maximum value.
    dir : int. Direction of the count.

    Notes:

    The out() method is bypassed. Counter's signal can not be sent 
    to audio outs.

    See also: Select

    Examples:
    
    >>> s = Server().boot()
    >>> s.start()
    >>> m = Metro(.125).play()
    >>> c = Counter(m, min=3, max=8, dir=2, mul=100)
    >>> a = Sine(freq=c, mul=.5).out()
    
    """
    def __init__(self, input, min=0, max=100, dir=0, mul=1, add=0):
        self._input = input
        self._min = min
        self._max = max
        self._dir = dir
        self._mul = mul
        self._add = add
        self._in_fader = InputFader(input)
        in_fader, min, max, dir, mul, add, lmax = convertArgsToLists(self._in_fader, min, max, dir, mul, add)
        self._base_objs = [Counter_base(wrap(in_fader,i), wrap(min,i), wrap(max,i), wrap(dir,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def out(self, chnl=0, inc=1):
        return self

    def setInput(self, x, fadetime=0.05):
        """
        Replace the `input` attribute.
        
        Parameters:

        x : PyoObject
            New signal to process.
        fadetime : float, optional
            Crossfade time between old and new input. Defaults to 0.05.

        """
        self._input = x
        self._in_fader.setInput(x, fadetime)
        
    def setMin(self, x):
        """Replace the `min` attribute.
        
        Parameters:

        x : int
            new `min` attribute.
        
        """
        self._min = x
        x, lmax = convertArgsToLists(x)
        [obj.setMin(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setMax(self, x):
        """Replace the `max` attribute.
        
        Parameters:

        x : int
            new `max` attribute.
        
        """
        self._max = x
        x, lmax = convertArgsToLists(x)
        [obj.setMax(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setDir(self, x):
        """Replace the `dir` attribute.
        
        Parameters:

        x : int {0, 1, 2}
            new `dir` attribute.
        
        """
        self._dir = x
        x, lmax = convertArgsToLists(x)
        [obj.setDir(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None):
        print "There is no control for Counter object."

    #def demo():
    #    execfile(DEMOS_PATH + "/Counter_demo.py")
    #demo = Call_example(demo)

    def args():
        return('Counter(input, min=0, max=100, dir=0, mul=1, add=0)')
    args = Print_args(args)

    @property
    def input(self): return self._input
    @input.setter
    def input(self, x): self.setInput(x)
    @property
    def min(self): return self._min
    @min.setter
    def min(self, x): self.setMin(x)
    @property
    def max(self): return self._max
    @max.setter
    def max(self, x): self.setMax(x)
    @property
    def dir(self): return self._dir
    @dir.setter
    def dir(self, x): self.setDir(x)

class Select(PyoObject):
    """
    Sends trigger on matching integer values.
    
    Select takes in input an audio signal containing integer numbers
    and sends a trigger when the input matches `value` parameter. This
    object is especially designed to be used with Counter object.

    Parent class: PyoObject

    Parameters:

    input : PyoObject
        Audio signal. Must contains integer numbers.
    value : int, optional
        Value to be matched to send a trigger. Defaults to 0.
    
    Methods:

    setInput(x, fadetime) : Replace the `input` attribute.
    setValue(x) : Replace the `value` attribute.

    Attributes:
    
    input : PyoObject. Audio signal.
    value : int. Matching value.

    Notes:

    The out() method is bypassed. Select's signal can not be sent 
    to audio outs.

    Select has no `mul` and `add` attributes.

    See also: Counter
    
    Examples:
    
    >>> s = Server().boot()
    >>> s.start()
    >>> env = HannTable()
    >>> m = Metro(.125).play()
    >>> te = TrigEnv(m, env, .125)
    >>> c = Counter(m, min=0, max=4)
    >>> se = Select(c, 0)
    >>> tr = TrigRand(se, 400, 600)
    >>> a = Sine(freq=tr, mul=te).out()
    
    """
    def __init__(self, input, value=0):
        self._input = input
        self._value = value
        self._in_fader = InputFader(input)
        in_fader, value, lmax = convertArgsToLists(self._in_fader, value)
        self._base_objs = [Select_base(wrap(in_fader,i), wrap(value,i)) for i in range(lmax)]

    def out(self, chnl=0, inc=1):
        return self

    def setMul(self, x):
        pass
        
    def setAdd(self, x):
        pass    

    def setInput(self, x, fadetime=0.05):
        """
        Replace the `input` attribute.
        
        Parameters:

        x : PyoObject
            New signal to process.
        fadetime : float, optional
            Crossfade time between old and new input. Defaults to 0.05.

        """
        self._input = x
        self._in_fader.setInput(x, fadetime)
        
    def setValue(self, x):
        """Replace the `value` attribute.
        
        Parameters:

        x : int
            new `value` attribute.
        
        """
        self._value = x
        x, lmax = convertArgsToLists(x)
        [obj.setValue(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None):
        print "There is no control for Select object."

    #def demo():
    #    execfile(DEMOS_PATH + "/Select_demo.py")
    #demo = Call_example(demo)

    def args():
        return('Select(input, value=0)')
    args = Print_args(args)

    @property
    def input(self): return self._input
    @input.setter
    def input(self, x): self.setInput(x)
    @property
    def value(self): return self._value
    @value.setter
    def value(self, x): self.setValue(x)
