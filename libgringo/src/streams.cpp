// Copyright (c) 2009, Roland Kaminski <kaminski@cs.uni-potsdam.de>
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

#include <gringo/streams.h>
#include <gringo/exceptions.h>
#include <boost/filesystem.hpp>

Streams::Streams(const StringVec &files, std::istream *constants)
{
	open(files, constants);
}

void Streams::open(const StringVec &files, std::istream *constants)
{
	if(constants) appendStream(constants, new std::string("<constants>"));
	foreach(const std::string &filename, files)
		addFile(filename);
	if (files.empty())
		appendStream(new std::istream(std::cin.rdbuf()), new std::string("<stdin>"));
}

void Streams::addFile(const std::string &filename)
{
	// handle stdin
	if(filename == "-")
	{
		if(!unique(filename)) return;
		appendStream(new std::istream(std::cin.rdbuf()), new std::string("<stdin>"));
	}
	// handle files
	else
	{
		std::string *path = 0;

		// create path relative to current file
		if(currentFilename() != "<stdin>")
		{
			boost::filesystem::path file(currentFilename());
			file.remove_leaf();
			file /= boost::filesystem::path(filename).leaf();
			if(boost::filesystem::exists(file)) path = new std::string(file.string());
		}

		// create path relative to current directory
		if(!path) path = new std::string(filename);

		// create and add stream
		if(!unique(*path)) return;
		appendStream(new std::ifstream(path->c_str()), path);
		if(!streams_.back().first->good())
		{
			throw FileException(streams_.back().second.c_str());
		}
	}
}

void Streams::appendStream(std::istream *stream, std::string *name)
{
	streams_.push(StreamPair(StreamPair::first_type(stream), *name));
}

bool Streams::next()
{
	streams_.pop();
	return !streams_.empty();
}

std::istream &Streams::currentStream()
{
	return *streams_.front().first;
}

const std::string &Streams::currentFilename()
{
	return streams_.front().second;
}

bool Streams::unique(const std::string &file)
{
	if(file.compare("-") == 0) return files_.insert(file).second;
	else return files_.insert(boost::filesystem::system_complete(boost::filesystem::path(file)).string()).second;
}
