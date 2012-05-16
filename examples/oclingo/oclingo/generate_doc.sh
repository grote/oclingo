mkdir doc
epydoc --output=doc/ --html --exclude=ply --exclude=Parser --no-private -v --no-frames .
