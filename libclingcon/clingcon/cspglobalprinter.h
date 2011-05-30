
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
 /*
    class PlainConstraintVarCondPrinter : public ConstraintVarCond::Printer
    {
    public:
            PlainConstraintVarCondPrinter(PlainOutput *output) : output_(output) { }
            void begin(ConstraintVarCond::GroundedConstraintVarLit& head)
            {
                head.vt_.print(output_->storage(), output_->out());
                output_->out() << "[";
                head.vweight_.print(output_->storage(), output_->out());
                output_->out() << "]";

            }

            void step(bool last)
            {
                if (!last)
                    output_->out() << ",";
            }




            PlainOutput *output() const { return output_; }
            std::ostream &out() const { return output_->out(); }

            virtual ~PlainConstraintVarCondPrinter(){}
    private:
            PlainOutput *output_;
    };

    class PlainConstraintHeadLitPrinter : public GlobalConstraintHeadLit::Printer
    {
    public:
            PlainConstraintHeadLitPrinter(PlainOutput *output) : output_(output) { }

            void begin()
            {
                output_->out() << "[";
            }

            void end()
            {
                output_->out() << "]";
            }
            ~PlainConstraintHeadLitPrinter(){}



            PlainOutput *output() const { return output_; }
            std::ostream &out() const { return output_->out(); }
    private:
            PlainOutput *output_;
    };
*/

    class PlainGlobalConstraintPrinter : public GlobalConstraint::Printer
    {
    public:
            PlainGlobalConstraintPrinter(PlainOutput *output) : output_(output) { }

            void type(GCType type)
            {
                output_->beginRule();
                switch(type)
                {
                case DISTINCT: output_->out() << "$distinct";
                break;
                case BINPACK: output_->out() << "$binpack";
                break;
                default: assert(false);
                }
                output_->print();
            }

            void beginHead()
            {
                output_->out() << "[";
            }

            void addHead(GroundedConstraintVarLitVec &vec)
            {
                for(size_t i = 0; i < vec.size(); ++i)
                {

                    vec[i].vt_.print(output_->storage(), output_->out());
                    output_->out() << "[";
                    vec[i].vweight_.print(output_->storage(), output_->out());
                    output_->out() << "]";
                    if (i+1<vec.size())
                        output_->out() << ",";
                }

            }

            void endHead()
            {
                output_->out() << "]";
            }

            void end()
            {
                output_->endRule();
            }

            ~PlainGlobalConstraintPrinter(){}



            PlainOutput *output() const { return output_; }
            std::ostream &out() const { return output_->out(); }
    private:
            PlainOutput *output_;
    };
    /*

    class LparseConstraintVarCondPrinter : public ConstraintVarCond::Printer
    {
    public:
            PlainConstraintVarCondPrinter(LparseConverter *output) : output_(output) { }
            void begin(ConstraintVarCond::GroundedConstraintVarLit& head)
            {
                head.vt_.print(output_->storage(), output_->out());
                output_->out() << "[";
                head.vweight_.print(output_->storage(), output_->out());
                output_->out() << "]";

            }

            void step(bool last)
            {
                if (!last)
                    output_->out() << ",";
            }




            LparseConverter *output() const { return output_; }
            std::ostream &out() const { return output_->out(); }

            virtual ~PlainConstraintVarCondPrinter(){}
    private:
            LparseConverter *output_;
    };

    class LParseConstraintHeadLitPrinter : public GlobalConstraintHeadLit::Printer
    {
    public:
            PlainConstraintHeadLitPrinter(LparseConverter *output) : output_(output) { }

            void begin()
            {
                output_->out() << "[";
            }

            void end()
            {
                output_->out() << "]";
            }
            ~PlainConstraintHeadLitPrinter(){}



            LparseConverter *output() const { return output_; }
            std::ostream &out() const { return output_->out(); }
    private:
            LparseConverter *output_;
    };

*/
    /*
    class LParseGlobalConstraintPrinter : public GlobalConstraint::Printer
    {
    public:
            LParseGlobalConstraintPrinter(LparseConverter *output) : output_(output) { }

            void begin(GlobalConstraint::Kind type)
            {
                type_ = type;
            }

            void heads(boost::ptr_vector<GlobalConstraintHeadLit>* vec, Grounder* g)
            {
                switch (type_)
                {
                    case GlobalConstraint::DISTINCT:
                    {
                        if (vec->size()!=1)
                            assert(fasle && "DISTINCT should not have more than one head.");
                        addHead(vec[0]);
                    }
                }
            }

            void addHead(GlobalConstraintHeadLit* hl)
            {

            }


            ~LParseGlobalConstraintPrinter(){}

            LparseConverter *output() const { return output_; }
            std::ostream &out() const { return output_->out(); }
    private:
            LparseConverter *output_;
            GlobalConstraint::Kind type_;
    };

*/


    class LParseGlobalConstraintPrinter : public GlobalConstraint::Printer
    {
    public:
            struct GC
            {
                GCType type_;
                boost::ptr_vector<IndexedGroundConstraintVec> heads_;
            };

            typedef boost::ptr_vector<GC> GCvec;

            LParseGlobalConstraintPrinter(LparseConverter *output) : output_(output) { }

            void type(GCType type)
            {
                tempGC_ = new GC();
                tempGC_->type_ = type;
            }

            void beginHead()
            {

            }

            void addHead(GroundedConstraintVarLitVec &vec)
            {
                IndexedGroundConstraintVec* newVec = new IndexedGroundConstraintVec();
                for (GroundedConstraintVarLitVec::const_iterator i = vec.begin(); i != vec.end(); ++i)
                {
                    newVec->push_back(new IndexedGroundConstraint(output_->storage(),i->vt_,i->vweight_));

                }
                tempGC_->heads_.push_back(newVec);
            }

            void endHead()
            {

            }

            void end()
            {
                if (tempGC_->type_==BINPACK)
                {
                    if (tempGC_->heads_[1].size() != tempGC_->heads_[2].size())
                        throw CSPException("In the binpack constraint, the number of items and sizes must be equal.");
                    for (size_t i = 0; i < tempGC_->heads_[0].size(); ++i)
                    {
                        std::stringstream ss(tempGC_->heads_[0][i].getWeight());
                        unsigned int w1;
                        if( (ss >> w1).fail() || w1 != i)
                          {
                              throw CSPException("In the binpack constraint, the indices of the bins must be consecutive integers, starting with 0.");
                          }
                    }

                    for (size_t i = 0; i < tempGC_->heads_[1].size(); ++i)
                    {
                        if (tempGC_->heads_[1][i].getWeight() != tempGC_->heads_[2][i].getWeight())
                        {
                            throw CSPException("Index Mismatch in the Binpacking constraints. Items and sizes must have the same indices.");
                        }
                    }
                }
                constraints_.push_back(tempGC_);
                output_->printBasicRule(output_->symbol(), LparseConverter::AtomVec(), LparseConverter::AtomVec());

            }

            GCvec& getGlobalConstraints()
            {
                return constraints_;
            }






            ~LParseGlobalConstraintPrinter(){}



            LparseConverter *output() const { return output_; }
            std::ostream &out() const { return output_->out(); }
    private:
            LparseConverter *output_;
            GC* tempGC_;
            GCvec constraints_;

    };



 }
