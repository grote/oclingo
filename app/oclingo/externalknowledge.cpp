// Copyright (c) 2010, Torsten Grote <tgrote@uni-potsdam.de>
// Copyright (c) 2010, Roland Kaminski <kaminski@cs.uni-potsdam.de>
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

#include "externalknowledge.h"
#include "oclaspoutput.h"
#include <gringo/grounder.h>

ExternalKnowledge::ExternalKnowledge(Grounder* grounder, oClaspOutput* output, Clasp::Solver* solver)
	: grounder_(grounder)
	, output_(output)
	, solver_(solver)
	, debug_(false)
{
//	externals_per_step_.push_back(IntSet());
//	externals_per_step_.push_back(IntSet());

	socket_ = NULL;
	port_ = 25277;
	reading_ = false;
	new_input_ = false;

	post_ = new ExternalKnowledge::PostPropagator(this);
	my_post_ = true;
	solver_stopped_ = false;

	step_ = 0;
	controller_step_ = 1;
	model_ = false;
//	debug_ = grounder_->options().debug;
}

ExternalKnowledge::~ExternalKnowledge() {
	if(socket_) {
		socket_->close();
		delete socket_;
	}

	// only delete post_ if it does not belong to solver
	if(my_post_) delete post_;
}

void ExternalKnowledge::addPostPropagator() {
	my_post_ = false;
	solver_->addPost(post_);
}

void ExternalKnowledge::removePostPropagator() {
	solver_->removePost(post_);
	my_post_ = true;
}

void ExternalKnowledge::addExternal(uint32_t symbol) {
	std::cerr << "ADD EXTERNAL " << symbol << std::endl;
	externals_.push_back(symbol);
	to_freeze_.push_back(symbol);
	//externals_per_step_.at(step_).insert(uid);
}

VarVec* ExternalKnowledge::getFreezers() {
	return &to_freeze_; // pointer so it can be cleared right away
}

void ExternalKnowledge::startSocket(int port) {
	using boost::asio::ip::tcp;

	if(debug_) std::cerr << "Starting socket..." << std::endl;

	try {
		tcp::acceptor acceptor(io_service_, tcp::endpoint(tcp::v4(), port));
		socket_ = new tcp::socket(io_service_);
		acceptor.accept(*socket_);

		if(debug_) std::cerr << "Client connected..." << std::endl;
	}
	catch (std::exception& e) {
		std::cerr << "Warning: " << e.what() << std::endl;
	}
}

void ExternalKnowledge::sendModel(std::string model) {
	std::stringstream ss;
	ss << "Step: " << step_ << "\n" << model;

	sendToClient(ss.str());

	model_ = true;
}

bool ExternalKnowledge::hasModel() {
	return model_;
}

void ExternalKnowledge::sendToClient(std::string msg) {
	if(not socket_) startSocket(port_);

	try {
		boost::asio::write(*socket_, boost::asio::buffer(msg+char(0)), boost::asio::transfer_all());
	}
	catch (std::exception& e) {
		std::cerr << "Warning: Could not send answer set. " << e.what() << std::endl;
	}
}

int ExternalKnowledge::poll() {
	io_service_.reset();
	int result = io_service_.poll_one();
	if(result) {
		std::cerr << "Polled for input and started " << result << " handler." << std::endl;
		if(solver_stopped_) {
			// solver was already stopped, don't try to stop it again
			result = 0;
		}
		solver_stopped_ = true;
	}
	return result;
}

void ExternalKnowledge::get() {
	try {
		if(!reading_) {
			if(debug_) std::cerr << "Getting external knowledge..." << std::endl;

			sendToClient("Input:\n");

			std::cerr << "Reading from socket..." << std::endl;
			reading_ = true;
			boost::asio::async_read_until(*socket_, b_, char(0), boost::bind(&ExternalKnowledge::readUntilHandler, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
		}
	}
	catch (std::exception& e) {
		std::cerr << "Warning: " << e.what() << std::endl;
	}
	// solver can be stopped again
	solver_stopped_ = false;
}

void ExternalKnowledge::readUntilHandler(const boost::system::error_code& e, size_t bytesT) {
	(void)bytesT;

	if (!e)	{
		new_input_ = true;
		reading_ = false;
	}
	else if(e == boost::asio::error::eof)
		throw std::runtime_error("Connection closed cleanly by client.");
	else
		throw boost::system::system_error(e);
}

bool ExternalKnowledge::addInput() {
	if(model_)
		sendToClient("End of Step.\n");

	if(!new_input_ && model_ && step_ > 0) {
		io_service_.reset();
		io_service_.run_one();
	}

	if(new_input_) {
		new_input_ = false;

		std::istream is(&b_);
		OnlineParser parser(output_, &is);
		parser.parse();

		if(parser.isTerminated()) {
			socket_->shutdown(boost::asio::ip::tcp::socket::shutdown_send);
			return false;
		}
	}
	return true;
}

void ExternalKnowledge::addStackPtr(GroundProgramBuilder::StackPtr stack) {
	stacks_.push_back(stack);
}

bool ExternalKnowledge::checkHead(uint32_t symbol) {
	// check if head atom has been defined as external
	if(find(externals_.begin(), externals_.end(), symbol) != externals_.end()) {
		return true;
	}
	else return false;
}

void ExternalKnowledge::addHead(uint32_t symbol) {
	// first remove head atom from externals
	VarVec::iterator it = find(externals_.begin(), externals_.end(), symbol);
	assert(it != externals_.end()); // call checkHead() first
	externals_.erase(it);

	// don't freeze added head and unfreeze it if necessary
	it = find(to_freeze_.begin(), to_freeze_.end(), symbol);
	if(it != to_freeze_.end()) {
		std::cerr << "REMOVED FROM FREEZE LIST " << symbol << std::endl;
		to_freeze_.erase(it);
	} else {
		output_->unfreezeAtom(symbol);
	}
}

void ExternalKnowledge::addPrematureKnowledge() {
	// check for knowledge from previous steps and add it if found
	if(controller_step_ == step_ && stacks_.size() > 0) {
		std::istream is(&b_);
		OnlineParser parser(output_, &is);
		while(stacks_.size()) {
			parser.add(GroundProgramBuilder::StackPtr(stacks_.pop_front().release()));
		}
	}
}

void ExternalKnowledge::setControllerStep(int step) {
	std::cerr << "SET CONTROLLER STEP TO " << step << std::endl;
	controller_step_ = step;
}

bool ExternalKnowledge::needsNewStep() {
	std::cerr << "DO I NEED NEW STEP? " << controller_step_ << " > " << step_;
	std::cerr << " RULE STACK SIZE: " << stacks_.size() << std::endl;
	return
			step_ == 0 ||				// is first iteration
			controller_step_ > step_ ||	// controller wants to progress step count
			stacks_.size() > 0;			// rule stack not empty
}

bool ExternalKnowledge::controllerNeedsNewStep() {
	return controller_step_ > step_;
}

VarVec& ExternalKnowledge::getExternals() {
	return externals_;
}

void ExternalKnowledge::endIteration() {
	// set model to false not only after completed step, but also after iterations
	model_ = false;

	std::cerr << "Controller Step: " << controller_step_ << std::endl;
}

void ExternalKnowledge::endStep() {
	// add previously added premature facts
	if(controller_step_ <= step_) {
/*
		for(std::vector<NS_OUTPUT::Atom*>::iterator i = premature_facts_.begin(); i!= premature_facts_.end(); ++i) {
			if(checkFact(*i)) {
				addNewFact(*i);
			} else {
				std::stringstream emsg;
				emsg << "Warning: Fact added for step " << controller_step_ << " has not been declared external and is now lost.";
				std::cerr << emsg.str() << std::endl;
				sendToClient(emsg.str());
				delete *i;
			}
		}
		premature_facts_.clear();
*/
	}

	endIteration();
/*
	externals_old_.swap(externals_);
	externals_.clear();

	for(UidValueMap::iterator i = externals_.begin(); i != externals_.end(); ++i) {
		// freeze new external atoms, facts have been deleted already in addNewAtom()
		output_->printExternalRule(i->second, i->first.first);
	}
*/
	step_++;
	std::cerr << "Step: " << step_ << std::endl;
//	externals_per_step_.push_back(IntSet());
}

void ExternalKnowledge::forgetExternals(int step) {
	(void) step;
	// unfreeze old externals for simplification by clasp
/*
	if(step > externals_per_step_.size()-1) {
		step = externals_per_step_.size()-1;
	}

	for(int i = 0; i <= step; ++i) {
		for(IntSet::iterator ext = externals_per_step_.at(i).begin(); ext != externals_per_step_.at(i).end(); ++ext) {
			if(eraseUidFromExternals(&externals_old_, *ext) > 0) {
				// external was still old, so needs unfreezing
				output_->unfreezeAtom(*ext);
			}
		}
		externals_per_step_.at(i).clear();
	}
*/
}
/*
// TODO is there any better way to do it?
int ExternalKnowledge::eraseUidFromExternals(UidValueMap* ext, int uid) {
	std::vector<UidValueMap::iterator> del;

	for(UidValueMap::iterator i = ext->begin(); i != ext->end(); ++i) {
		if(i->second == uid) {
			del.push_back(i);
		}
	}

	for(std::vector<UidValueMap::iterator>::iterator i = del.begin(); i != del.end(); ++i) {
		ext->erase(*i);
	}

	return del.size();
}
*/

//////////////////////////////////////////////////////////////////////////////
// ExternalKnowledge::PostPropagator
//////////////////////////////////////////////////////////////////////////////

ExternalKnowledge::PostPropagator::PostPropagator(ExternalKnowledge* ext) {
	ext_ = ext;
}

bool ExternalKnowledge::PostPropagator::propagate(Clasp::Solver &s) {
	if(ext_->poll()) {
		std::cerr << "Stopping solver..." << std::endl;
		s.setStopConflict();
	}

	if(s.hasConflict())	{
		assert(s.hasConflict());
		return false;
	}
	return true;
}
