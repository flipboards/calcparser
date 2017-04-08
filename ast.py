import re

class Token(object):
    TYPES = set(['var', 'op', 'ket', 'func'])

    def __init__(self, type, value):
        self.type = type
        self.value = value

    def __eq__(self, other):
        return self.type == other.type and self.value == other.value

class SymbolAST(object):
    PRECEDENCE = {
        '+':10,
        '-':10,
        '*':11,
        '/':11,
        '^':12,
        '=':2,
        '<':8,
        '>':8,
        ',':-1,
        '(':0
        }


    def __init__(self):
        self.name = None
        self.precedence = None

    def check_return(self):
        return isinstance(self, Function) or self.name == '='

    def unparse(self, assign_prefix='&'):
        """ ast -> string
        """
        if isinstance(self, Var):
            return self.name
        elif isinstance(self, BinOp):   # infix
            if self.name == '=':
                return assign_prefix + self.left.unparse()[1:] + self.name + self.right.unparse()
            else:
                return self.left.unparse() + self.name + self.right.unparse()
        elif isinstance(self, Function):
            return self.name + '(' + ','.join([arg.unparse() for arg in self.args]) + ')'


class BinOp(SymbolAST):
    OPS = set(['+', '-', '*', '/', '^', '=', ',', '<', '>'])

    def __init__(self, op, left=None, right=None):
        self.left = left
        self.right = right
        if op in BinOp.OPS:
            self.name = op
            self.precedence = SymbolAST.PRECEDENCE[op]
        else:
            raise RuntimeError('Operation %s not found' % op)


class Var(SymbolAST):
    def __init__(self, name):
        self.name = name


class Function(SymbolAST):
    def __init__(self, name):
        self.name = name
        self.precedence = 0
        self.args = []

def GenerateTokens(line_str, var_name_table):
    """ string -> list of tokens
    """
    token_list = []
    tmp_num = ''    # word temp (\w)
    tmp_sym = ''    # simple temp (\W)

    for i in line_str:
        if i == '(':
            if tmp_num: # function
                token_list.append(Token('func', tmp_num))
                tmp_num = ''
            elif tmp_sym:
                token_list.append(Token('op', tmp_sym))
                tmp_sym = ''
                token_list.append(Token('func', ''))
            else:
                token_list.append(Token('func', ''))

        elif i == ')':
            if tmp_num:
                if tmp_num.isdigit():
                    token_list.append(Token('var', tmp_num))
                else:
                    token_list.append(Token('var', var_name_table[tmp_num]))
                tmp_num = ''
            elif tmp_sym:
                token_list.append(Token('op', tmp_sym))
                tmp_sym = ''
            token_list.append(Token('ket', i))

        elif not re.match('\w|&|@|$', i): # is a symbol (\W)
            if tmp_num:
                if tmp_num.isdigit():
                    token_list.append(Token('var', tmp_num))
                else:
                    token_list.append(Token('var', var_name_table[tmp_num]))
                tmp_num = ''
            tmp_sym += i

        else:   # is a variable
            if tmp_sym:
                token_list.append(Token('op', tmp_sym))
                tmp_sym = ''
            tmp_num += i

    if tmp_num:
        if tmp_num.isdigit():
            token_list.append(Token('var', tmp_num))
        else:
            token_list.append(Token('var', var_name_table[tmp_num]))
    elif tmp_sym:
        token_list.append(Token('op', tmp_sym))
    return token_list


def CompileLine(line_str, var_name_table):
    """ String -> AST
    """

    token_list = GenerateTokens(line_str, var_name_table)
    stack_sym = [] 
    stack_num = [] # stack of ast nodes (trees)

    for token in token_list:

        if token.type == 'func':
            stack_sym.append(Function(token.value))

        elif token.type == 'ket':
            while not isinstance(stack_sym[-1], Function):
                right_child = stack_num.pop()
                left_child = stack_num.pop()
                stack_sym_top = stack_sym.pop()
                stack_sym_top.left = left_child
                stack_sym_top.right = right_child
                stack_num.append(stack_sym_top)

            stack_sym_top = stack_sym.pop() # a function
            stack_num_top = stack_num.pop() # could be a ',' or variable/operator
            
            # iterate through comma series
            comma_iterator = stack_num_top
            while comma_iterator.name == ',':
                stack_sym_top.args.append(comma_iterator.left)
                comma_iterator = comma_iterator.right
            stack_sym_top.args.append(comma_iterator)
            stack_num.append(stack_sym_top)

        elif token.type == 'var':
            stack_num.append(Var(token.value))

        elif token.type == 'op':

            new_sym = BinOp(token.value)
            while stack_sym and stack_sym[-1].precedence >= new_sym.precedence:
                right_child = stack_num.pop()
                left_child = stack_num.pop()
                stack_sym_top = stack_sym.pop()
                stack_sym_top.left = left_child
                stack_sym_top.right = right_child
                stack_num.append(stack_sym_top)

            stack_sym.append(new_sym)

    while len(stack_sym) > 0:
        right_child = stack_num.pop()
        left_child = stack_num.pop()
        stack_sym_top = stack_sym.pop()
        stack_sym_top.left = left_child
        stack_sym_top.right = right_child
        stack_num.append(stack_sym_top)

    if not len(stack_num) == 1:
        raise RuntimeError('Stack not match')

    return stack_num.pop()


def GenerateControlTokens(line_str):
    idx = 0
    token_list = []

    while idx < len(line_str):
        bracket_match = ControlAST.pattern.search(line_str, idx)
        if not bracket_match:
            token_list.append(Token('block', line_str[idx:]))
            break
        token_list.append(Token('block', line_str[idx:bracket_match.start()]))
        if bracket_match.group() == '}':
            token_list.append(Token('ket', '}'))
        else:
            token_list.append(Token('control', bracket_match.group()[:-1]))
        idx = bracket_match.end()
    return token_list


def split_small_braket(line):
    """ a(b)c => a, b
    """
    try:
        argleft = line.index('(')
    except ValueError:
        return line, ''
    else:
        argright = line.rfind(')')
        return line[:argleft], line[argleft+1:argright]


def CompileControlBlock(line_str):
    token_list = GenerateControlTokens(line_str)
    stack_control = [] # list of tokens

    for token in token_list:

        if token.type == 'control':
            name, condition = split_small_braket(token.value)
            stack_control.append(RawControlBlock(name, condition))
            
        elif token.type == 'ket':
            children = []
            while not isinstance(stack_control[-1], RawControlBlock):
                children.append(stack_control.pop())

            children.reverse()
            stack_control_top = stack_control.pop()
            if stack_control_top.name == 'if':
                stack_control.append(IfBlock(stack_control_top.condition, children))
            elif stack_control_top.name == 'while':
                stack_control.append(WhileBlock(stack_control_top.condition, children))
            elif stack_control_top.name == 'else':
                if not isinstance(stack_control[-1], NormalBlock) or stack_control[-1].condition != '':
                    raise RuntimeError('There cannot be thing between "if" and "else"!')
                stack_control.pop()
                if not isinstance(stack_control[-1], IfBlock):
                    raise RuntimeError('"else" has no if!')
                stack_control[-1].children2 = children

        else:
            stack_control.append(NormalBlock(token.value))

    return RootBlock(stack_control)


class ControlAST(object):
    pattern = re.compile('}|\w+(\(.*?\))?{')
    _cur_label = 0
    CONTROLS = set(['if', 'else', 'while'])

    def __init__(self, name, condition):
        self.name = name
        self.condition = condition


class RawControlBlock(ControlAST):
    def __init__(self, name, condition):
        if name not in ControlAST.CONTROLS:
            raise RuntimeError('name %s is not defined!' % name)

        return super().__init__(name, condition)

class WhileBlock(ControlAST):
    def __init__(self, condition, children):
        self.children = children
        return super().__init__('while', condition)


    def untie(self):
        self.label_start = '_lbl%d' % ControlAST._cur_label
        ControlAST._cur_label += 1
        self.label_end = '_lbl%d' % ControlAST._cur_label
        ControlAST._cur_label += 1
       
        result = '%s:cngoto %s %s;' % (self.label_start, self.label_end, self.condition)
        result += ''.join([child.untie() for child in self.children])
        result += 'goto %s;%s:' % (self.label_start, self.label_end)
        return result

class IfBlock(ControlAST):
    def __init__(self, condition, children):
        self.children = children
        self.children2 = []
        return super().__init__('if', condition)

    def untie(self):
        self.label_else = '_lbl%d' % ControlAST._cur_label
        ControlAST._cur_label += 1
        
        result = 'cngoto %s %s;' % (self.label_else, self.condition)
        result += ''.join([child.untie() for child in self.children])
        if self.children2:
            result += 'goto %s;' % self.label_else
            result += ''.join([child.untie() for child in self.children2])
        result += '%s:' % self.label_else
        return result

class RootBlock(ControlAST):
    def __init__(self, children):
        self.children = children
        return super().__init__('root', '')

    def untie(self):
        return ''.join([child.untie() for child in self.children])


class NormalBlock(ControlAST):
    def __init__(self, condition):
        return super().__init__('', condition)

    def untie(self):
        return self.condition

