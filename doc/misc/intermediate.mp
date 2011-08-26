input metauml;

beginfig(1);

	% Statements
	Class.statement("Statement")()();
	classStereotypes.statement("<<interface>>");

	Class.rule("Rule")()();
	Class.optimize("Optimize")()();
	Class.display("Display")()();
	Class.external("External")()();

	Group.statements(rule, external, display, optimize);
	leftToRight.midy(20)(rule, display, optimize, external);
	topToBottom.midx(30)(statement, statements);

	% Literals
	Class.lit("Lit")()();
	classStereotypes.lit("<<interface>>");

	Class.optimizeLit("OptimizeLit")()();
	Class.conditionalLit("ConditionalLit")()();
	Class.simpleLit("SimpleLit")()();
	Class.aggrLit("AggrLit")()();
	Class.predLit("PredLit")()();
	Class.boolLit("BoolLit")()();
	% TODO: conditional literal

	Group.lits(lit, simpleLit, aggrLit, predLit, boolLit, optimizeLit, conditionalLit);
	topToBottom.midx(30)(optimizeLit, simpleLit, boolLit);
	rightToLeft.midy(20)(optimizeLit, lit);
	leftToRight.midy(20)(boolLit, predLit);
	rightToLeft.midy(30)(simpleLit, aggrLit);
	topToBottom.midx(30)(aggrLit, conditionalLit);

	topToBottom.midx(30)(statements, lits);

	% Draw Stuff
	drawObjects(statement, statements, lits);

	link(inheritance)(rule.n -- statement.w);
	link(inheritance)(optimize.n -- (statement.se + statement.s) / 2);
	link(inheritance)(display.n -- (statement.sw + statement.s) / 2);
	link(inheritance)(external.n -- statement.e);

	link(inheritance)(aggrLit.n -- lit.s);
	link(inheritance)(simpleLit.nw -- lit.se);
	link(inheritance)(boolLit.n -- simpleLit.s);
	link(inheritance)((predLit.n + predLit.nw) / 2 -- simpleLit.e);

	link(composition)(aggrLit.s -- conditionalLit.n);
	item(iAssoc)("*")(obj.sw = conditionalLit.n + (4, 0));

	link(association)(conditionalLit.ne -- simpleLit.w);
	item(iAssoc)("*")(obj.se = simpleLit.w);

	link(association)(rule.s -- (lit.nw + lit.n) / 2);
	item(iAssoc)("*")(obj.sw = (lit.nw + lit.n) / 2);

	link(association)(display.s -- (lit.n + lit.ne) / 2);
	item(iAssoc)("*")(obj.sw = (lit.n + lit.ne) / 2);

	link(association)(optimizeLit.w -- lit.e);
	item(iAssoc)("*")(obj.sw = lit.e);

	link(composition)(optimize.s -- optimizeLit.n);
	item(iAssoc)("*")(obj.sw = optimizeLit.n + (6, 0));

	link(association)(external.s -- (predLit.ne + predLit.n) / 2);
	item(iAssoc)("*")(obj.sw = (predLit.ne + predLit.n) / 2);

endfig;

end

