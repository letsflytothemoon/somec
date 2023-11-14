#pragma once
#include <stack>
#include "someasm.h"

template <class COpts>
class SomeC
{
public:
    typedef SomeASM<COpts> ASM;
    
    static void mov_r_r(int l, int r)
    //this must be placed in some compiling strategies
    {
        ASM::mov_r_r(l, r);
        //or
        //ASM::push(r);
        //ASM::pop(l);
    }
    
    static int& StackVarsCount()
    {
        static int value = 0;
        return value;
    }
    
    struct RegVar
    {
        int regNum;
        
        static int& RegVarsCount()
        {
            static int value;
            return value;
        }
        
        static int Allocate()
        {
            int regNum = RegVarsCount()++ % COpts::REGS;
            if(RegVarsCount() > COpts::REGS)
            {
                StackVarsCount()++;
                ASM::push_r(regNum);
            }
            return regNum;
        }
        
        static void Free(int regNum)
        {
            if(regNum >= COpts::REGS)
            {
                StackVarsCount()--;
                ASM::pop_r(regNum);
            }
            RegVarsCount()--;
        }
        
        RegVar() : regNum(Allocate()) {}
        
        RegVar(const RegVar& init) : regNum(Allocate())
        { mov_r_r(regNum, init.regNum); }
        
        RegVar(int init) : regNum(Allocate())
        { ASM::mov_r_b(regNum, init); }
        
        ~RegVar()
        { Free(regNum); }
        
        RegVar& operator =(const RegVar& value)
        {
            mov_r_r(regNum, value.regNum);
            return *this;
        }
    };
    
    struct StackStrat
    {
        typedef Id int;
        
        int Position(int id) const
        { return StackVarsCount() - id; }
        
        static int Allocate()
        {
            ASM::push_r(0);
            return StackVarsCount()++;
        }
        
        static int Allocate(const RegVar& value)
        {
            ASM::push_r(value.regNum);
            return StackVarsCount()++;
        }
        
        static void Free(int)
        {
            //need to add pop instruction with no argument
            //without storing to reg
            RegVar regVar;
            ASM::pop_r(regVar.regNum);
            StackVarsCount()--;
        }
        
        static RegVar Value(int id)
        {
            RegVar result;
            ASM::mov_r_s(result.regNum, Position(id));
            return result;
        }
        
        static RegVar Address(int id)
        {
            RegVar result;
            ASM::sp_to_r(result.regNum, id);
            return result;
        }
        
        static void Set(int id, const RegVar& value)
        { ASM::mov_s_r(Position(id), value.regNum); }
    };
    
    struct StaticStrat
    {
        typedef Id int;
        
        static int Allocate()
        { return ASM::create_static_var(); }
        
        static int Allocate(const RegVar& value)
        {
            int id = ASM::create_static_var();
            RegVar address;
            ASM::mov_r_static_var_address(address.regNum, id);
            ASM::mov_a_r(address.regNum, value.regNum);
            return id;
        }
        
        static void Free(int)
        {}
        
        static RegVar Value(int id)
        {
            RegVar result;
            ASM::mov_r_static_var_address(result.regNum, id);
            ASM::mov_r_a(result.regNum, result.regNum);
            return result;
        }
        
        static RegVar Address(int id)
        {
            RegVar result;
            ASM::mov_r_static_var_address(result.regNum, id);
            return result;
        }
        
        void Set(int id, const RegVar& value)
        {
            RegVar address;
            ASM::mov_r_static_var_address(address.regNum, id);
            ASM::mov_a_r(address.regNum, value.regNum);
        }
    };
    
    
    
    
    template <class IdT = int>
    struct Ref
    {
        typedef Id IdT;
        
        static Id Allocate() { throw "cannot be just created, its a reference you stupid"; }
        
        static Id Allocate(Target& target)
        { return target.Adress(); }//totally wrong
        
        static void Free(const Id&) {}
        
        static RegVar Value(const Id& id)
        { return StoringStrat::Value(id); }
        
        static void Set(const Id& id, const RegVar& value)
        { StoringStrat::Set(id, value); }
    }
    
    template <class IdT
    
    
    
    template <class StoringStrat>
    class Var
    {
        template <class T> friend class Ref;
        StoringStrat::Id id;
    public:
        Var() : id(StoringStrat::Allocate())
        { }
        
        Var(const Var<StoringStrat>& other) :
        storingStrat((const RegVar&)other)
        
        template <class T>
        Var(const T& init) :
        storingStrat((const RegVar&)init)
        { }
        
        operator RegVar() const
        { return storingStrat; }
        
        Var<StoringStrat>& operator=(const Var<StoringStrat>& value)
        {
            storingStrat.Set(value);
            return *this;
        }
        
        template <class T>
        Var<StoringStrat>& operator=(const T& value)
        {
            storingStrat.Set(value);
            return *this;
        }
        
        
        
        
    };
    
    struct OpAdd
    {
        void operator()(int l, int r) { ASM::add_r_r(l, r); }
        void jump_if_false(int label_num) { ASM::jeq_label(label_num); }
    };
    
    template <class Op, class L, class R>
    struct BinOp
    {
        const L& l;
        const R& r;
    public:
        BinOp(const L& l, const R& r) : l(l), r(r) {}
        operator RegVar() const
        {
            RegVar result(l);
            Op op;
            op(result.regNum, ((const RegVar&)r).regNum);
            return result;
        }
    };
    
    template <class L, class R> friend BinOp<OpAdd, L, R> operator+(const L& l, const R& r)
    { return BinOp<OpAdd, L, R>(l, r); }
    
    template <class Condition, class Action>
    static void _if(const Condition& condition, Action action)
    {
        int label = ASM::create_label();
        {
            const RegVar& conditionRegVar(condition);
            ASM::and_r_r(conditionRegVar.regNum, conditionRegVar.regNum);
        }
        ASM::jeq_label(label);
        action();
        ASM::label(label);
    }
    
    template <class Condition, class Action, class ElseAction>
    static void _if(const Condition& condition, Action action, ElseAction elseAction)
    {
        int elseLabel = ASM::create_label();
        int exitLabel = ASM::create_label();
        {
            const RegVar& conditionRegVar(condition);
            ASM::and_r_r(conditionRegVar.regNum, conditionRegVar.regNum);
        }
        ASM::jeq_label(elseLabel);
        action();
        ASM::jmp_label(exitLabel);
        ASM::label(elseLabel);
        elseAction();
        ASM::label(exitLabel);
    }
    
    static std::stack<Var<StackStrat>>& BreakFlagsStack()
    {
        static std::stack<Var<StackStrat>> value;
        return value;
    }
    
    template <class Condition, class Action>
    static void _while(const Condition& condition, Action action)
    {
        BreakFlagsStack().emplace();
        int loopLabel = ASM::create_label();
        int exitLabel = ASM::create_label();
        ASM::label(loopLabel);
        {
            const RegVar& breakFlagRegVar(BreakFlagsStack().top());
            ASM::and_r_r(breakFlagRegVar.regNum, breakFlagRegVar.regNum);
        }
        ASM::jne_label(exitLabel);
        {
            const RegVar& conditionRegVar(condition);
            ASM::and_r_r(conditionRegVar.regNum, conditionRegVar.regNum);
        }
        ASM::jeq_label(exitLabel);
        action();
        ASM::jmp_label(loopLabel);
        ASM::label(exitLabel);
        BreakFlagsStack().pop();
    }
    
    static void _break()
    {
        BreakFlagsStack().front() = 1;
    }
};










