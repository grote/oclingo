// Copyright (c) 2010, Torsten Grote <tgrote@uni-potsdam.de>
//
// This file is part of gringo.
//
// gringo is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// gringo is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with gringo.  If not, see <http://www.gnu.org/licenses/>.

#pragma once

#include <gringo/gringo.h>
#include <gringo/val.h>
#include <gringo/groundprogrambuilder.h>
#include <gringo/lparseconverter.h>

#include <oclingo/onlineparser.h>

#include <clasp/solver.h>
#include <clasp/constraint.h>

#include <boost/ptr_container/ptr_list.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>

class oClaspOutput;

class ExternalKnowledge
{
public:
	ExternalKnowledge(Grounder* grounder, oClaspOutput* output, Clasp::Solver* solver, uint32_t port, bool import);
	~ExternalKnowledge();
	void addPostPropagator();
	void removePostPropagator();
	void addExternal(uint32_t symbol);
	void startSocket(int port);
	void sendModel(std::string);
	bool hasModel();
	void sendToClient(std::string msg);
	int poll();
	void get();
	void readUntilHandler(const boost::system::error_code& e, size_t bytesT);
	bool addInput();
	void addStackPtr(GroundProgramBuilder::StackPtr stack);
	void savePrematureVol(bool volatile, int window);
	bool checkHead(LparseConverter::Symbol const &sym);
	void addHead(uint32_t symbol);
	bool addPrematureKnowledge();
	int getStep();
	void setControllerStep(int step);
	int getControllerStep();
	bool needsNewStep();
	VarVec& getExternals();
	void endIteration();
	void endStep();
	void forgetExternals(uint32_t step);

protected:
	struct PostPropagator : public Clasp::PostPropagator {
	public:
		PostPropagator(ExternalKnowledge* ext);
		bool propagate(Clasp::Solver &s);
		uint32 priority() const { return Clasp::PostPropagator::priority_lowest - 1; }
	private:
		ExternalKnowledge* ext_;
	};

private:
	Grounder* grounder_;
	oClaspOutput* output_;
	Clasp::Solver* solver_;

	typedef boost::ptr_list<GroundProgramBuilder::Stack> StackPtrList;
	StackPtrList stacks_;
	std::list<std::pair<bool,int> > vol_stack_;

	// externals handling
	VarVec externals_;
	VarVec to_freeze_;
	std::vector<VarVec> externals_per_step_;
	uint32_t forget_;
	bool import_;

	// socket stuff
	boost::asio::io_service io_service_;
	boost::asio::ip::tcp::socket* socket_;
	boost::asio::streambuf b_;
	int port_;
	bool reading_;
	bool new_input_;

	PostPropagator* post_;
	bool my_post_;
	bool solver_stopped_;

	int step_;
	int controller_step_;
	bool model_;
	bool debug_;
};
