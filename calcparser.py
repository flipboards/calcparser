import subprocess as sp
import os
import re
import sys
import getopt
import ast

global_variables = []

def split_large_braket(line):
    """ a(b){c}d => a, b, c
    """
    argleft = line.index('(')
    bodyleft = line.index('{')
    argright = line.rfind(')', None, bodyleft)
    if argright == -1:
        raise RuntimeError('Bracket not match')
    bodyright = line.rfind('}')
    if bodyright == -1:
        raise RuntimeError('Bracket not match')

    return line[:argleft], line[argleft+1:argright], line[bodyleft+1:bodyright]


def sstr(arg, size=6):
    """ Normalize int to str.
        Arg: int;
    """
    return '0'*(size - len(str(arg))) + str(arg)


def clear_line(line):
    """ Remove additional spaces from a string.
    """
    return re.sub(' +', ' ', line.strip(' '))


def check_name(name_str):
    """ check if a name is valid.
    """
    if len(name_str) == 0 or re.match('\d', name_str) or not re.match('\w|&|@', name_str):
        raise RuntimeError('Name %s not valid!' % name_str)

    if re.search('\W', name_str[1:]):
        raise RuntimeError('Name %s is not valid!' % name_str)


def parse_value(value):
    """ Check value describe a number.
    """
    try:
        return float(value)
    except ValueError:
        raise RuntimeError('%s is not a number!' % value)


def parse_line(line):
    """ Assuming functions are all in a line. Parse a line.
        Argument: string describe a intergrated instruction (set function / set variables);
        Returns: list of string;
    """

    linesplit = clear_line(line.strip('\n')).split(' ', 2)

    # line must begin with set.
    if len(linesplit) < 2 or linesplit[0] != 'set':
        raise RuntimeError('Bad instruction.')

    # function: name without bracket
    if '(' in linesplit[1]:
        if len(linesplit) == 3:
            function_name, function_argstr, function_body = split_large_braket(linesplit[1] + ' ' + linesplit[2])
        else:
            function_name, function_argstr, function_body = split_large_braket(linesplit[1])

        check_name(function_name)
        function_argstr = re.sub(' +', '', function_argstr)
        if function_argstr != '':
            function_args = function_argstr.split(',')     
            for farg in function_args:
                check_name(farg)
        else:
            function_args = []

        fp = FunctionParser()
        fp.global_variables = global_variables
        local_var_count, returns, function_body = fp.parse_function(function_args, function_body)
        return 'setf %s:%d,%d,%d ' % (function_name, len(function_args), local_var_count, returns) + function_body

   # global variable
    else:
        if len(linesplit) < 3:
            raise RuntimeError('Global variable not initalized.')
        check_name(linesplit[1])
        if not linesplit[2] or linesplit[2][-1] != ';':
            raise RuntimeError('Comma ";" should be in the line end!')
        value = parse_value(linesplit[2][:-1])
        global_variables.append(linesplit[1])
        return 'setv %s %f' % (linesplit[1], value)


class FunctionParser(object):

    JUMP_LABEL = '___jmp'

    def __init__(self, **kwargs):
        self.local_variables = []
        self.returns = False
        self.label_dst = {}      # label -> line_idx
        self.jump_table = {}     # line_idx -> label
        self.global_variables = []

        return super().__init__(**kwargs)


    def strip_label(self, lines):
        self.label_dst = {'_end': len(lines)}

        lines_after_labelstrip  = []
        for line_idx, line in enumerate(lines):
            line = line.strip(' ')
            match_label = re.match(".*?(?=:)", line)
            while match_label:
                check_name(match_label.group())
                if match_label.group() in self.label_dst:
                    raise RuntimeError('label %s already exist!' % match_label.group())
                self.label_dst[match_label.group()] = line_idx
                line = line[len(match_label.group()) + 1:].lstrip(' ')
                match_label = re.match(".*(?=:)", line)
            else:
                lines_after_labelstrip.append(line)
        return lines_after_labelstrip

    def prehandle(self, lines):
        lines_after_prehandling = []
        label_marker = {}
        for line_idx, line in enumerate(self.strip_label(lines)):
            if not line:
                continue
            if re.match('set ', line):
                local_var_name = line[4:]
                check_name(local_var_name)
                self.local_variables.append(local_var_name)
            elif re.match('return ', line):
                lines_after_prehandling.append(
                    '$r=' + re.sub(' ','',line[7:])
                    )
                returns = True
                lines_after_prehandling.append('goto _end')
            elif line == 'return':
                lines_after_prehandling.append('goto _end')
            elif re.match('goto ', line):
                lines_after_prehandling.append(line)
            elif re.match('cngoto ', line):
                lines_after_prehandling.append(line)
            else:
                lines_after_prehandling.append(re.sub(' ', '', line))
            label_marker[line_idx] = len(lines_after_prehandling) - 1

        label_marker[len(lines)] = len(lines_after_prehandling)
        for label in self.label_dst:
            self.label_dst[label] = label_marker[self.label_dst[label]]
        return lines_after_prehandling

    def parse_lines(self, lines_after_connection):
        # prepare variable name table
        var_name_table = {}
        var_name_table[FunctionParser.JUMP_LABEL] = FunctionParser.JUMP_LABEL
        var_name_table['$r'] = '$r'

        for idx, farg in enumerate(self.function_args):
            var_name_table[farg] = '$a%d' % (idx + 1)
            var_name_table['&' + farg] = '&a%d' % (idx + 1)
            var_name_table['@' + farg] = '@a%d' % (idx + 1)

        for gvar in self.global_variables:
            var_name_table[gvar] = gvar

        for idx, lvar in enumerate(self.local_variables):
            var_name_table[lvar] = '$l%d' % idx
            var_name_table['&' + lvar] = '&l%d' % idx
            var_name_table['@' + lvar] = '@l%d' % idx

        # in-line parsing
        lines_after_tree = []
        for line_idx in range(len(lines_after_connection)):
        
            symbolast = ast.CompileLine(lines_after_connection[line_idx], var_name_table)
            if not line_idx in self.jump_table and not symbolast.check_return():
                raise RuntimeError('Invalid line %d' % line_idx)

            lines_after_tree.append(symbolast.unparse())

        return lines_after_tree

    def fill_jumpdst(self, lines_after_tree):
        # prepare char counts
        charcount_cusum = [0]
        for line in lines_after_tree:
            charcount_cusum.append(charcount_cusum[-1] + len(line) + 1)
    
        label_charcount = {}
        for label in self.label_dst:
            label_charcount[label] = charcount_cusum[self.label_dst[label]]

        # replace jump mark
        function_output = ''
        for line_idx, line in enumerate(lines_after_tree):
            if line_idx in self.jump_table:          
                function_output += re.sub(FunctionParser.JUMP_LABEL, sstr(label_charcount[self.jump_table[line_idx]]), line) + ':'
            else:
                function_output += line + ';'
        return function_output

    def collect_label(self, lines_after_strech):
        # replace goto
        lines_after_connection = []

        # record and remove goto
        for line_idx, line in enumerate(lines_after_strech):
            if re.match('goto ', line):
                if line[5:] not in self.label_dst:
                    raise RuntimeError('label %s not found!' % line[5:])

                self.jump_table[line_idx] = line[5:]
                lines_after_connection.append(FunctionParser.JUMP_LABEL)

            elif re.match('cngoto ', line):
                ls = line.split(' ', 3)
                if not len(ls) == 3:
                    raise RuntimeError('"cngoto" must have dst and condition!')
                if ls[1] not in self.label_dst:
                    raise RuntimeError('label %s not found!' % ls[1])
                self.jump_table[line_idx] = ls[1]
                lines_after_connection.append('(%s)*(0-1-%s)+%s' % (re.sub(' ','',ls[2]), FunctionParser.JUMP_LABEL, FunctionParser.JUMP_LABEL))
            else:
                lines_after_connection.append(line)

        return lines_after_connection

    def parse_function(self, function_args, function_body):
        """ Function-body: list of string.
        """
        self.function_args = function_args

        # parse control strings
        controlast = ast.CompileControlBlock(function_body)
        lines_after_strech = controlast.untie().split(';')

        # prehandling
        lines_after_prehandling = self.prehandle(lines_after_strech)

        # replace goto
        lines_after_connection = self.collect_label(lines_after_prehandling)

        lines_after_tree = self.parse_lines(lines_after_connection)

        function_output = self.fill_jumpdst(lines_after_tree)

        return len(self.local_variables), self.returns, function_output


def parse_file(inputname, outputname):
    ofile = open(outputname, 'w')
    
    with open(inputname, 'r') as ifile:
        input = ''.join([line.strip('\n') for line in ifile.readlines()])
        blocks = []
        bracket_stack = []
        last = 0
        for i in range(len(input)):
            if input[i] == '{':
                if last < i and not bracket_stack:
                    blocks.append(input[last:i].strip(' '))
                bracket_stack.append(i)
            elif input[i] == '}':
                start = bracket_stack.pop()
                if not bracket_stack:
                    blocks.append(input[start:i+1])
                    last = i+1
        if last < len(input):
            blocks.append(input[last:])

        if bracket_stack:
            raise RuntimeError('bracket not match!')

        lines = []
        function_name = ''
        for block in blocks:
            if not block:
                continue
            if block[0] == '{':
                lines.append(function_name + block)
            else:
                bs = block.split(';')
                if block[-1] != ';':
                    function_name = bs[-1]
                    lines += [bs_ + ';' for bs_ in bs[:-1]]
                else:
                    function_name = ''
                    lines += [bs_ + ';' for bs_ in bs]
        
        for line in lines:
            ofile.write(parse_line(line) + '\n')
    ofile.close()


def main():
    if len(sys.argv) < 1:
        print('please choose a file!')

    parse_file(sys.argv[1], sys.argv[1].rsplit('.',1)[0]+'.calc')

if __name__ == '__main__':
    main()

