"""
Module oClingo Formatter
TODO
"""

import re


def formatList(as_list):
	"""
	Formats a list of answer set predicates as a table
	"""
	i = 1
	result = ''
	for answer_set in as_list:
		result += "Answer: %d\n" % i
		result += _formatAnswerSet(answer_set) + "\n"
		i += 1

	return result


def _formatAnswerSet(answer_set):
	plan = {}
	pred_exp = re.compile("^-?[a-z_][a-zA-Z0-9_]*\((.*?),?(\d+)\)$")

	# stores all predicates in dictionary plan
	for predicate in answer_set:
		match = pred_exp.match(predicate)
		if match != None:
			t = int(match.group(2))
			if t in plan:
				plan[t].append(predicate)
			else:
				plan[t] = [predicate]
		else:
			if 'B' in plan:
				plan['B'].append(predicate)
			else:
				plan['B'] = [predicate]
	
	# get predicate lenghts
	row_len = {}
	for time in plan:
		plan[time].sort()
		row = 0
		for predicate in plan[time]:
			if row in row_len:
				if len(predicate) > row_len[row]:
					row_len[row] = len(predicate)
			else:
				row_len[row] = len(predicate)
			row += 1
	
	result = ''

	# format predicates in rows
	if 'B' in plan:
		result += _formatRow(plan, row_len, 'B')
	for time in sorted(plan.iterkeys()):
		if time != 'B':
			result += _formatRow(plan, row_len, time)

	return result


def _formatRow(plan, row_len, time):
	result = ''
	time_str = str(time).rjust(len(str(len(plan))))
	result += "  %s. " % time_str
	row = 0
	for predicate in plan[time]:
		 result += predicate.ljust(row_len[row]+1)
		 row += 1
	
	return result + "\n"

