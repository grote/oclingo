import sys, re
import ply.lex as lex, ply.yacc as yacc

class Parser:
	"""
	TODO
	"""

	def __init__(self, input):
		self.input = input
		self.online_input = []


	def __get_value_type(self, value):
		if re.match(r'\d+', value):
			return 'NUM'
		elif re.match(r'"(\w+|\\")+"', value):
			return 'STR'
		return 'ID'


	def __return_p(self, p):
		result = ''
		for token in p:
			if token != None:
				result += token
		return result
	
	def get_online_input(self):
		return self.online_input

	def parse_input(self):
		"""
		Parses the input and returns the program tree if successful
		"""
		self.lexer = lex.lex(module=self)
		yacc.yacc(module=self)
		return yacc.parse(self.input)


	# token definitions
	tokens = ('ID', 'NUM', 'STR', 'LBRAC', 'RBRAC',	'COMMA', 'NOT', 'IF',
			  'DOT', 'DOTS', 'COLON', 'MINUS', 'REL', 'STEP', 'ENDSTEP', 'FORGET',
			  'CUMULATIVE', 'VOLATILE', 'ASSERT', 'RETRACT', 'STOP')
	
	t_NUM = r'\d+'
	t_STR = r'"(\w+|\\")+"'
	t_LBRAC = r'\('
	t_RBRAC = r'\)'
	t_COMMA = r','
	t_IF = r':-'
	t_DOT = r'\.'
	t_DOTS = r'\.\.'
	t_COLON = r':'
	t_MINUS = r'-'
	t_REL = r'==|!=|(>|<)=?'

	t_STEP = r'\#step'
	t_ENDSTEP = r'\#endstep'
	t_FORGET = r'\#forget'
	t_CUMULATIVE = r'\#cumulative'
	t_VOLATILE = r'\#volatile'
	t_ASSERT = r'\#assert'
	t_RETRACT = r'\#retract'
	t_STOP = r'\#stop'

	def t_NOT(self, t):
		r'not'
		return t

	def t_WS(self, t):
		r'[ \t\r]+'
		pass

	def t_NL(self, t):
		r'\n+'
		t.lexer.lineno += len(t.value)

	def t_COMMENT(self, t):
		r'%[^\n]*\n'
		t.lexer.lineno += 1

	def t_ID(self, t):
		r'[a-z][a-zA-Z0-9_]*'
		return t

	# Error handling rule
	def t_error(self, t):
		sys.stderr.write("Illegal character '%s' on line %d.\n" % (t.value[0], t.lexer.lineno))
		t.lexer.skip(1)

	# grammar definition

	def p_program(self, p):
		# maybe no stop necessary, then take blocks here directly
		'''
		program : blocks STOP DOT
		'''

	def p_blocks(self, p):
		'''		
		blocks : start ENDSTEP DOT
			   | start expression ENDSTEP DOT
			   | blocks start ENDSTEP DOT
			   | blocks start expression ENDSTEP DOT
		'''
		p[0] = self.__return_p(p)
		
		# add line breaks for nicer output
		p[0] = p[0].replace('.','.\n')
		
		self.online_input.append(p[0])

		# clear old blocks
		p[0] = ''

	def p_start(self, p):
		'''
		start : STEP NUM DOT
			  | STEP NUM COLON NUM DOT
		'''
		if len(p) == 4:
			p[0] = p[1] + ' ' + p[2] + p[3]
		elif len(p) == 6:
			p[0] = p[1] + ' ' + p[2] + ' ' + p[3] + ' ' + p[4] + p[5]

	def p_expression(self, p):
		'''
		expression : statement DOT
				   | expression statement DOT
		'''
		p[0] = self.__return_p(p)

	def p_statement(self, p):
		'''
		statement : rule
				  | meta
		'''
		p[0] = self.__return_p(p)

	def p_meta(self, p):
		'''
		meta : CUMULATIVE
			 | VOLATILE
			 | VOLATILE COLON NUM
			 | FORGET NUM
			 | FORGET NUM DOTS NUM
			 | ASSERT COLON term
			 | RETRACT COLON term
		'''
		if len(p) == 3:
			# FORGET NUM
			p[0] = p[1] + ' ' + p[2]
		elif len(p) == 4:
			# VOLATILE COLON NUM
			# ASSERT COLON term
			# RETRACT COLON term
			p[0] = p[1] + ' ' + p[2] + ' ' + p[3]
		elif len(p) == 5:
			# FORGET NUM DOTS NUM
			p[0] = p[1] + ' ' + p[2] + p[3] + p[4]
		else:
			p[0] = self.__return_p(p)

	def p_rule(self, p):
		'''
		rule : predicate
			 | IF body
			 | predicate IF body
		'''
		if len(p) == 3:
			# IF body
			p[0] = p[1] + ' ' + p[2]
		elif len(p) == 4:
			# predicate IF body
			p[0] = p[1] + ' ' + p[2] + ' ' + p[3]
		else:
			p[0] = self.__return_p(p)

	def p_body(self, p):
		'''
		body : literal
			 | body COMMA literal
		'''
		p[0] = self.__return_p(p)

	def p_literal(self, p):
		'''
		literal : predicate
				| NOT predicate
				| relation
		'''
		if len(p) == 3:
			# NOT predicate
			p[0] = p[1] + ' ' + p[2]
		else:
			p[0] = self.__return_p(p)

	def p_predicate(self, p):
		'''
		predicate : ID
				  | ID LBRAC args RBRAC
				  | MINUS ID
				  | MINUS ID LBRAC args RBRAC
		'''
		p[0] = self.__return_p(p)

	def p_relation(self, p):
		'''
		relation : term REL term
		'''
		p[0] = self.__return_p(p)

	def p_args(self, p):
		'''
		args : term
			 | args COMMA term
		'''
		p[0] = self.__return_p(p)

	def p_term(self, p):
		'''
		term : ID
			 | STR
			 | ID LBRAC args RBRAC
			 | NUM
		'''
		p[0] = self.__return_p(p)

	def p_error(self, p):
		if p:
			sys.stderr.write("Syntax error '%s' at line %d.\n" % (p.value, p.lexer.lineno))
		else:
			sys.stderr.write("Syntax error at EOF.\n")

