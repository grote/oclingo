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

ExternalKnowledge::ExternalKnowledge(Grounder* grounder, oClaspOutput* output, Clasp::Solver* solver, uint32_t port, bool import)
	: grounder_(grounder)
	, output_(output)
	, solver_(solver)
	, socket_(NULL)
	, port_(port)
	, reading_(false)
	, new_input_(false)
	, my_post_(true)
	, solver_stopped_(false)
	, import_(import)
	, step_(1)
	, controller_step_(1)
	, model_(true)
	, debug_(false)
{
	post_ = new ExternalKnowledge::PostPropagator(this);

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

			if(not socket_) {
				std::cerr << "Waiting for connection on port " << port_ << " from controller..." << std::endl;
			} else{
				std::cerr << "Reading from socket..." << std::endl;
			}
			std::cerr.flush();

			sendToClient("Input:\n");

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

	if(!new_input_ && model_) {
		io_service_.reset();
		io_service_.run_one();
	}

	output_->unfreezeOldQueryAtoms();

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

// always call with addStackPtr()
void ExternalKnowledge::savePrematureVol(OnlineParser::Part part, int window=0) {
	vol_stack_.push_back(std::make_pair(part, window));
}

/*
bool ExternalKnowledge::checkHead(LparseConverter::Symbol const &sym) {
	// check if head atom has been defined as external
	if(!sym.external || find(externals_.begin(), externals_.end(), sym.symbol) == externals_.end()) {
		std::ostringstream emsg;
		emsg << "Warning: Head ";
		sym.print(output_->storage(), emsg);
		emsg << " has not been declared external.";

		if(!import_) {
			emsg << " The entire rule will be ignored. Try starting oclingo with --import=all";
		}
		std::cerr << emsg.str() << std::endl;
		sendToClient(emsg.str());
		if(!import_) return false;
	}
	return true;
}

void ExternalKnowledge::addHead(uint32_t symbol) {
	// first remove head atom from externals
	VarVec::iterator it = find(externals_.begin(), externals_.end(), symbol);
	// only continue if head has been defined as external
	if(it != externals_.end()) {
		externals_.erase(it); // TODO do we want to keep it if this is a volatile rule?

		// don't freeze added head or unfreeze it if necessary
		it = find(to_freeze_.begin(), to_freeze_.end(), symbol);
		if(it != to_freeze_.end()) {
			to_freeze_.erase(it);
		} else {
			output_->unfreezeAtom(symbol);
		}
	}
}
*/

bool ExternalKnowledge::addPrematureKnowledge() {
	assert(stacks_.size() == vol_stack_.size());

	bool added = false;
	// check for knowledge from previous steps and add it if found
	if(controller_step_ == step_ && stacks_.size() > 0) {
		added = true;

		std::istream is(&b_);
		OnlineParser parser(output_, &is);
		while(stacks_.size()) {
			// add part information to parser
			parser.setPart(vol_stack_.front().first);
			parser.setVolatileWindow(vol_stack_.front().second);
			vol_stack_.pop_front();

			parser.add(GroundProgramBuilder::StackPtr(stacks_.pop_front().release()));
			// return to adding cummulative rules
			parser.setPart(OnlineParser::CUMULATIVE);
		}
	}

	// TODO check if premature #forget statement is handled properly

	return added;
}

void ExternalKnowledge::setControllerStep(int step) {
	controller_step_ = step;
}

int ExternalKnowledge::getControllerStep() {
	return controller_step_;
}

bool ExternalKnowledge::needsNewStep() {
//	std::cerr << "  new step? " << controller_step_ << " > " << step_ << std::endl;
	return
			controller_step_ > step_ ||	// controller wants to progress step count
			stacks_.size() > 0;			// rule stack not empty
}

void ExternalKnowledge::endIteration() {
	// set model to false not only after completed step, but also after iterations
	model_ = false;
}

void ExternalKnowledge::endStep() {
	endIteration();

	step_++;
	std::cerr << "Step: " << step_ << std::endl;
}

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
