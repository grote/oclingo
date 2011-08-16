#define BOOST_TEST_MODULE GringoTest
#include <boost/test/unit_test.hpp>

#include <gringo/grounder.h>
#include <gringo/parser.h>
#include <gringo/inclit.h>
#include <gringo/streams.h>
#include "clingo/claspoutput.h"
#include <clasp/unfounded_check.h>
#include <clasp/solve_algorithms.h>
#include <clasp/model_enumerators.h>
#include <gringo/plainoutput.h>

#include <cstdarg>

struct Tester : public Clasp::Enumerator::Report
{
	Tester(std::string const &is, const char *x, ...)
	{
		// ground/solve
		{
			Clasp::ProgramBuilder pb;
			ClaspOutput o(false);
			TermExpansionPtr te(new TermExpansion());
			BodyOrderHeuristicPtr bo(new BasicBodyOrderHeuristic());
			Grounder g(&o, false, te, bo);
			Clasp::Solver s;
			Module *mb = g.createModule();
			Module *mc = g.createModule();
			mc->parent(mb);
			Module *mv = g.createModule();
			mv->parent(mc);
			IncConfig ic;
			Streams in;
			Parser p(&g, mb, mc, mv, ic, in, false, false);
			Streams::StreamPtr sp(new std::stringstream(is));
			in.appendStream(sp, "<test>");
			o.setProgramBuilder(&pb);
			pb.startProgram(ai, new Clasp::DefaultUnfoundedCheck());
			o.initialize();
			p.parse();
			g.analyze();
			g.ground(*mb);
			g.ground(*mc);
			g.ground(*mv);
			o.finalize();
			pb.endProgram(s, true);
			Clasp::SolveParams csp;
			csp.setEnumerator(new Clasp::RecordEnumerator(this));
			csp.enumerator()->init(s, 0);
			Clasp::solve(s, csp);
		}
		// check
		{
			uint32_t n = 0;
			va_list vl;
			va_start(vl, x);
			while(x)
			{
				Model m;
				while(x)
				{
					m.insert(std::string(x));
					x = va_arg(vl, const char *);
				}
				BOOST_CHECK(std::find(models.begin(), models.end(), m) != models.end());
				n++;
				x = va_arg(vl, const char *);
			}
			BOOST_CHECK(models.size() == n);
			va_end(vl);
		}
	}

	void reportModel(const Clasp::Solver& s, const Clasp::Enumerator& self)
	{
		models.push_back(Model());
		for (Clasp::AtomIndex::const_iterator it = ai.begin(); it != ai.end(); it++)
		{
			if (!it->second.name.empty() && s.isTrue(it->second.lit))
			{
				models.back().insert(it->second.name);
			}
		}
	}

	void debug()
	{
		foreach (Model const &m, models)
		{
			std::cerr << "Model:";
			foreach (std::string const &s, m)
			{
				std::cerr << " " << s;
			}
			std::cerr << std::endl;
		}
	}

	typedef std::set<std::string> Model;

	Clasp::AtomIndex ai;
	std::vector<Model> models;
};

BOOST_AUTO_TEST_CASE( simple_test )
{
	Tester
	(
		"a :- not b.\n"
		"b :- not a.\n",

		"a", NULL,
		"b", NULL,
		NULL
	);
}

BOOST_AUTO_TEST_CASE( aggr_test )
{
	// Note: failed because of bug in lparse aggregate translation
	Tester
	(
		"location(1..3)."
		"box(a;b;c)."
		"1 { loc(B,L) : location(L) } 1 :- box(B)."
		":- { not loc(B,L) } 0, { not loc(B1,L) } 0, B != B1, location(L), box(B;B1).",

		"box(a)", "box(b)", "box(c)", "location(1)", "location(2)", "location(3)", "loc(c,3)", "loc(b,2)", "loc(a,1)", NULL,
		"box(a)", "box(b)", "box(c)", "location(1)", "location(2)", "location(3)", "loc(c,3)", "loc(b,1)", "loc(a,2)", NULL,
		"box(a)", "box(b)", "box(c)", "location(1)", "location(2)", "location(3)", "loc(c,2)", "loc(b,3)", "loc(a,1)", NULL,
		"box(a)", "box(b)", "box(c)", "location(1)", "location(2)", "location(3)", "loc(c,2)", "loc(b,1)", "loc(a,3)", NULL,
		"box(a)", "box(b)", "box(c)", "location(1)", "location(2)", "location(3)", "loc(c,1)", "loc(b,3)", "loc(a,2)", NULL,
		"box(a)", "box(b)", "box(c)", "location(1)", "location(2)", "location(3)", "loc(c,1)", "loc(b,2)", "loc(a,3)", NULL,
		NULL
	);
}

BOOST_AUTO_TEST_CASE( company1 )
{
	Tester
	(
		"controls(X,Y) :- 51 #sum+ [ owns(X,Y,S) = S, owns(Z,Y,S) = S : controls(X,Z) : X != Y ], company(X), company(Y), X != Y."
		"company(c1;c2;c3;c4)."
		"owns(c1,c2,60). owns(c1,c3,20). owns(c2,c3,40). owns(c3,c4,51).",

		"controls(c3,c4)", "controls(c1,c2)", "controls(c1,c3)", "controls(c1,c4)",
		"owns(c1,c2,60)", "owns(c1,c3,20)", "owns(c2,c3,40)", "owns(c3,c4,51)",
		"company(c1)", "company(c2)", "company(c3)", "company(c4)", NULL,

		NULL
	);
}

BOOST_AUTO_TEST_CASE( company2 )
{
	Tester
	(
		"controls(X,Y) :- 51 #sum [ owns(X,Y,S) = S, owns(Z,Y,S) = S : controls(X,Z) : X != Y ], company(X), company(Y), X != Y."
		"company(c1;c2;c3;c4)."
		"owns(c1,c2,60). owns(c1,c3,20). owns(c2,c3,40). owns(c3,c4,51).",

		"controls(c3,c4)", "controls(c1,c2)", "controls(c1,c3)", "controls(c1,c4)",
		"owns(c1,c2,60)", "owns(c1,c3,20)", "owns(c2,c3,40)", "owns(c3,c4,51)",
		"company(c1)", "company(c2)", "company(c3)", "company(c4)", NULL,

		NULL
	);
}

BOOST_AUTO_TEST_CASE( company3 )
{
	Tester
	(
		"controls(X,Y) :- not [ owns(X,Y,S) = S, owns(Z,Y,S) = S : controls(X,Z) : X != Y ] 50, company(X), company(Y), X != Y."
		"company(c1;c2;c3;c4)."
		"owns(c1,c2,60). owns(c1,c3,20). owns(c2,c3,40). owns(c3,c4,51).",

		"controls(c3,c4)", "controls(c1,c2)", "controls(c1,c3)", "controls(c1,c4)",
		"owns(c1,c2,60)", "owns(c1,c3,20)", "owns(c2,c3,40)", "owns(c3,c4,51)",
		"company(c1)", "company(c2)", "company(c3)", "company(c4)", NULL,

		NULL
	);
}

BOOST_AUTO_TEST_CASE( company4 )
{
	Tester
	(
		"controls(X,Y)  :- owns(X,Y,_), 51 #sum+ [ owns(X,Y,S) = S ]."
		"reach(X,Z)     :- controls(X,Y), owns(Y,Z,_), 51 [ owns(_,Z,S) = S ]."
		"controls(X,Z)  :- reach(X,Z), X != Z, 51 #sum+ [ owns(X,Z,S) = S, owns(Y,Z,S) = S : controls(X,Y) ]."
		"company(c1;c2;c3;c4)."
		"owns(c1,c2,60). owns(c1,c3,20). owns(c2,c3,40). owns(c3,c4,51).",

		"reach(c1,c3)", "reach(c1,c4)",
		"controls(c3,c4)", "controls(c1,c2)", "controls(c1,c3)", "controls(c1,c4)",
		"owns(c1,c2,60)", "owns(c1,c3,20)", "owns(c2,c3,40)", "owns(c3,c4,51)",
		"company(c1)", "company(c2)", "company(c3)", "company(c4)", NULL,

		NULL
	);
}

BOOST_AUTO_TEST_CASE( conjunction_test1 )
{
	Tester
	(
		"d(a;b)."
		"p(a,1..2)."
		"2 { p(b,1..2) } 2."
		"q(X) :- p(X,Y) : p(X,Y), d(X).",

		"d(a)", "d(b)",
		"p(a,1)", "p(a,2)", "q(a)",
		"p(b,1)", "p(b,2)", "q(b)", NULL,

		NULL
	);
}
