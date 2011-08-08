
#pragma once


#include <clingcon/constraintvarcond.h>
#include <gringo/plainoutput.h>
#include <clingcon/constraintterm.h>
#include <clingcon/globalconstraint.h>
#include <gringo/grounder.h>
#include <clingcon/exception.h>
#include <gringo/lparseconverter.h>


 namespace Clingcon
 {


    class PlainGlobalConstraintPrinter : public GlobalConstraint::Printer
    {
    public:
        PlainGlobalConstraintPrinter(PlainOutput *output) : output_(output), numHead_(0) { }

            void type(GCType type)
            {
                type_=type;
                output_->beginRule();
                switch(type)
                {
                case DISTINCT: output_->out() << "$distinct";
                break;
                case BINPACK: output_->out() << "$binpack";
                break;
                case COUNT_GLOBAL:
                case COUNT_UNIQUE:
                case COUNT: output_->out() << "$count";
                break;
                case MINIMIZE_SET:
                case MINIMIZE: output_->out() << "$minimize";
                break;
                case MAXIMIZE_SET:
                case MAXIMIZE: output_->out() << "$maximize";
                break;
                default: assert(false);
                }
                output_->print();
            }

            void beginHead()
            {
                if (type_==MINIMIZE_SET || type_==MAXIMIZE_SET )
                {
                    output_->out() << "{";
                    return;
                }
                if (type_==MINIMIZE || type_==MAXIMIZE )
                {
                    output_->out() << "[";
                    return;
                }

                if ((type_==COUNT || type_==COUNT_UNIQUE) && numHead_==1)
                    assert(false);
                else
                    if (type_==COUNT_GLOBAL && numHead_==0)
                    {
                        output_->out() << "{";
                    }
                    else
                        output_->out() << "[";
            }

            void beginHead(CSPLit::Type cmp)
            {
                std::ostream& out(output_->out());
                switch(cmp)
                {
                case CSPLit::ASSIGN:  assert(false); break;
                case CSPLit::GREATER: out << "$>"; break;
                case CSPLit::LOWER: out << "$<"; break;
                case CSPLit::EQUAL: out << "$=="; break;
                case CSPLit::GEQUAL: out << "$>="; break;
                case CSPLit::LEQUAL: out << "$<=";break;
                case CSPLit::INEQUAL: out << "$!="; break;
                default: assert(false);
                }
            }

            void addHead(GroundedConstraintVarLitVec &vec)
            {
                for(size_t i = 0; i < vec.size(); ++i)
                {

                    vec[i].vt_->print(output_->storage(), output_->out());
                    if (type_==COUNT && numHead_==0)
                    {
                            output_->out() << "==";
                            vec[i].vweight_.print(output_->storage(), output_->out());

                    }else
                    if (type_==COUNT && numHead_==1)
                    {
                        continue;
                    }else
                    if (type_==COUNT_UNIQUE && numHead_==0)
                    {
                        output_->out() << "$==";
                        vec[i].vweight_.print(output_->storage(), output_->out());

                    }else
                    if (type_==COUNT_GLOBAL && numHead_==0)
                    {

                    }else
                    if (type_==COUNT_GLOBAL && numHead_==1)
                    {
                        output_->out() << "[";
                        vec[i].vweight_.print(output_->storage(), output_->out());
                        output_->out() << "]";

                    }
                    if (type_==MINIMIZE_SET || type_==MAXIMIZE_SET || type_==MINIMIZE || type_==MAXIMIZE )
                    {
                        output_->out() << "@";
                        vec[i].vweight_.print(output_->storage(), output_->out());
                    }
                    else
                    {
                        //default
                        if (!(type_==COUNT_UNIQUE))
                        {
                        output_->out() << "[";
                        vec[i].vweight_.print(output_->storage(), output_->out());
                        output_->out() << "]";
                        }
                    }

                    if (i+1<vec.size())
                        output_->out() << ",";

                }

            }

            void endHead()
            {
                if (type_==MINIMIZE_SET || type_==MAXIMIZE_SET )
                {
                    output_->out() << "}";
                    return;
                }
                if (type_==MINIMIZE || type_==MAXIMIZE )
                {
                    output_->out() << "]";
                    return;
                }

                if ((type_==COUNT || type_==COUNT_UNIQUE)&& numHead_==1)
                    ;
                else
                    output_->out() << "]";

                numHead_++;
            }

            void end()
            {
                output_->endRule();
                numHead_=0;
            }

            ~PlainGlobalConstraintPrinter(){}



            PlainOutput *output() const { return output_; }
            std::ostream &out() const { return output_->out(); }
    private:
            PlainOutput *output_;
            GCType type_;
            size_t numHead_;
    };



    class LParseGlobalConstraintPrinter : public GlobalConstraint::Printer
    {
    public:
            struct GC
            {
                GCType type_;
                boost::ptr_vector<IndexedGroundConstraintVec> heads_;
                CSPLit::Type cmp_;
            };

            typedef boost::ptr_vector<GC> GCvec;

            LParseGlobalConstraintPrinter(LparseConverter *output) : output_(output), numHead_(0) { }

            void type(GCType type)
            {
                tempGC_ = new GC();
                tempGC_->type_ = type;
                switch(type)
                {
                case DISTINCT: out_ << "$distinct";
                break;
                case BINPACK: out_ << "$binpack";
                break;
                case COUNT_GLOBAL:
                case COUNT_UNIQUE:
                case COUNT: out_ << "$count";
                break;
                case MINIMIZE_SET:
                case MINIMIZE: out_ << "$minimize";
                break;
                case MAXIMIZE_SET:
                case MAXIMIZE: out_ << "$maximize";
                break;
                default: assert(false);
                }
            }

            void beginHead()
            {

                if (tempGC_->type_==MINIMIZE_SET || tempGC_->type_==MAXIMIZE_SET )
                {
                    out_ << "{";
                    return;
                }
                if (tempGC_->type_==MINIMIZE || tempGC_->type_==MAXIMIZE )
                {
                   out_ << "[";
                    return;
                }

                if ((tempGC_->type_==COUNT || tempGC_->type_==COUNT_UNIQUE) && numHead_==1)
                    assert(false);
                else
                    if (tempGC_->type_==COUNT_GLOBAL && numHead_==0)
                    {
                        out_ << "{";
                    }
                    else
                        out_ << "[";

            }

            void beginHead(CSPLit::Type cmp)
            {
                tempGC_->cmp_ = cmp;
                switch(cmp)
                {
                case CSPLit::ASSIGN:  assert(false); break;
                case CSPLit::GREATER: out_ << "$>"; break;
                case CSPLit::LOWER: out_ << "$<"; break;
                case CSPLit::EQUAL: out_ << "$=="; break;
                case CSPLit::GEQUAL: out_ << "$>="; break;
                case CSPLit::LEQUAL: out_ << "$<=";break;
                case CSPLit::INEQUAL: out_ << "$!="; break;
                default: assert(false);
                }
            }

            void addHead(GroundedConstraintVarLitVec &vec)
            {
                for(size_t i = 0; i < vec.size(); ++i)
                {

                    vec[i].vt_->print(output_->storage(), out_);
                    if (tempGC_->type_==COUNT && numHead_==0)
                    {
                            out_ << "==";
                            vec[i].vweight_.print(output_->storage(), out_);

                    }else
                    if (tempGC_->type_==COUNT && numHead_==1)
                    {
                        continue;
                    }else
                    if (tempGC_->type_==COUNT_UNIQUE && numHead_==0)
                    {
                       out_ << "$==";
                        vec[i].vweight_.print(output_->storage(), out_);

                    }else
                    if (tempGC_->type_==COUNT_GLOBAL && numHead_==0)
                    {

                    }else
                    if (tempGC_->type_==COUNT_GLOBAL && numHead_==1)
                    {
                        out_ << "[";
                        vec[i].vweight_.print(output_->storage(),out_);
                        out_ << "]";

                    }
                    if (tempGC_->type_==MINIMIZE_SET || tempGC_->type_==MAXIMIZE_SET || tempGC_->type_==MINIMIZE || tempGC_->type_==MAXIMIZE )
                    {
                        out_ << "@";
                        vec[i].vweight_.print(output_->storage(),out_);
                    }
                    else
                    {
                        //default
                        if (!(tempGC_->type_==COUNT_UNIQUE))
                        {
                            out_ << "[";
                            vec[i].vweight_.print(output_->storage(), out_);
                            out_ << "]";
                        }
                    }

                    if (i+1<vec.size())
                        out_ << ",";

                }


                IndexedGroundConstraintVec* newVec = new IndexedGroundConstraintVec();
                for (GroundedConstraintVarLitVec::iterator i = vec.begin(); i != vec.end(); ++i)
                {
                    newVec->push_back(new IndexedGroundConstraint(output_->storage(),i->vt_.release(),i->vweight_));
                }
                tempGC_->heads_.push_back(newVec);
            }

            void endHead()
            {
                if (tempGC_->type_==MINIMIZE_SET || tempGC_->type_==MAXIMIZE_SET )
                {
                    out_ << "}";
                    return;
                }
                if (tempGC_->type_==MINIMIZE || tempGC_->type_==MAXIMIZE )
                {
                    out_ << "]";
                    return;
                }

                if ((tempGC_->type_==COUNT || tempGC_->type_==COUNT_UNIQUE)&& numHead_==1)
                    ;
                else
                    out_ << "]";

                numHead_++;

            }

            void end()
            {
                if (tempGC_->type_==BINPACK)
                {
                    if (tempGC_->heads_[1].size() != tempGC_->heads_[2].size())
                        throw CSPException("In the binpack constraint, the number of items and sizes must be equal.");
                    for (size_t i = 0; i < tempGC_->heads_[0].size(); ++i)
                    {
                        if (!tempGC_->heads_[0][i].b_.isInteger() || tempGC_->heads_[0][i].b_.getInteger()!=i)
                        {
                            throw CSPException("In the binpack constraint, the indices of the bins must be consecutive integers, starting with 0.");
                        }
                    }

                    for (size_t i = 0; i < tempGC_->heads_[1].size(); ++i)
                    {
                        if ((tempGC_->heads_[1][i].b_) != (tempGC_->heads_[2][i].b_))
                        {
                            throw CSPException("Index Mismatch in the Binpacking constraints. Items and sizes must have the same indices.");
                        }
                    }
                }
                constraints_.push_back(tempGC_);
                 //uint32_t atom = static_cast<CSPOutputInterface*>(output_)->symbol(name,true);
                 output_->printBasicRule(static_cast<CSPOutputInterface*>(output_)->symbol(out_.str(), false), LparseConverter::AtomVec(), LparseConverter::AtomVec());
                // output_->printBasicRule(static_cast<CSPOutputInterface*>(output_)->symbol("test", true), LparseConverter::AtomVec(), LparseConverter::AtomVec());
                 numHead_=0;
                 out_.str("");


            }

            GCvec& getGlobalConstraints()
            {
                return constraints_;
            }






            ~LParseGlobalConstraintPrinter(){}



            LparseConverter *output() const { return output_; }
    private:
            LparseConverter *output_;
            GC* tempGC_;
            GCvec constraints_;
            std::stringstream out_;
            size_t numHead_;
    };



 }
