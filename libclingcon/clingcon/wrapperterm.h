#ifndef WRAPPERTERM_H
#define WRAPPERTERM_H

#include <gringo/term.h>
#include <clingcon/constraintterm.h>


namespace Clingcon
{
        class WrapperTerm
        {
        public:
                WrapperTerm(const Loc& loc) : loc_(loc)
		{}
                //WrapperTerm(const WrapperTerm& w);

                virtual ConstraintTerm* toConstraintTerm() = 0;
                virtual Term* toTerm() const = 0;

                virtual ~WrapperTerm(){}
                virtual WrapperTerm *clone() const = 0;
		virtual const Loc& loc() { return loc_; }
        protected:
                const Loc& loc_;
        };

        inline WrapperTerm* new_clone(const WrapperTerm& a)
        {
                        return a.clone();
        }

}

#endif // WRAPPERTERM_H
                               
