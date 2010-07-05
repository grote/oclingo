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

#pragma once

#include <gringo/gringo.h>

/** Input streams to be processed by the parser.
 * The streams are organized in a queue
 * and can be added directly or generated from a filename.
 */
class Streams
{
private:
	typedef std::set<std::string> StringSet;
	typedef std::pair<boost::shared_ptr<std::istream>, std::string> StreamPair;
	typedef std::queue<StreamPair> StreamQueue;
public:
	typedef std::auto_ptr<std::istream> StreamPtr;
	/**
	 * \param files Initial list of files to be opened by name.
	 *        If the list is empty or contains the name "-", stdin will be opened.
	 *        Duplicate files will be ignored.
	 * \param constants Definitions of constants to be added as first stream.
	 */
	Streams() { }
	Streams(const StringVec &files, StreamPtr constants = StreamPtr(0));

	void open(const StringVec &files, StreamPtr constants = StreamPtr(0));

	/** adds a file.
	 * \param filename The filename or "-" to open stdin.
	 */
	void addFile(const std::string &filename, bool relative = true);

	/** adds a stream.
	 * \note both stream and name will be deleted by Streams.
	 * \param stream The input stream.
	 * \param name The stream name that's printed on parsing errors.
	 */
	void appendStream(StreamPtr stream, const std::string &name);

	/** returns the current stream */
	std::istream &currentStream();

	/** returns the current stream name */
	const std::string &currentFilename();

	/** closes the current stream and selects the next.
	 * \returns whether there is another stream in the queue.
	 * \note Make sure to call this until all streams have been handled.
	 */
	bool next();

private:
	/** checks the uniqueness of the file (by name) */
	bool unique(const std::string &file);

	/** The set of filenames for uniqueness check */
	StringSet files_;

	/** The queued streams and their names */
	StreamQueue streams_;

	Streams(const Streams&);
	Streams& operator=(const Streams&);
};
