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
        ~RegVar() { Free(regNum); }
        RegVar& operator =(const RegVar& value)
        {
            mov_r_r(regNum, value.regNum);
            return *this;
        }
    };
    
    struct StackStrat
    {
        int num;
        int Position() const
        { return StackVarsCount() - num; }
        static int Allocate() { return StackVarsCount()++; }
        static void Free() { StackVarsCount()--; } //m.b. some check
        StackStrat() : num(Allocate())
        { ASM::push_r(0); }
        //StackStrat(const StackStrat& init) : num(Allocate())
        //{
        //    RegVar& regStrat(init);
        //    ASM::push_r(init.regNum);
        //}
        ~StackStrat()
        {
            Free();
            RegVar regStrat;
            ASM::pop_r(regStrat.regNum);
        }
        StackStrat(const RegVar& init) : num(Allocate())
        { ASM::push_r(init.regNum); }
        operator RegVar() const
        {
            RegVar result;
            ASM::mov_r_s(result.regNum, Position());
            return result;
        }
        void Set(const RegVar& value)
        { ASM::mov_s_r(Position(), value.regNum); }
    };
    
    struct StaticStrat
    {
        int relAddress;
        static int Allocate() { return ASM::create_static_var(); }
        StaticStrat() : relAddress(Allocate()) {}
        //StaticStrat(const StaticStrat& init) : relAddress(Allocate())
        //{
        //    RegVar regStrat;
        //    ASM::mov_r_static_var_address(regStrat.regNum, init.relAddress);
        //    ASM::mov_r_a(regStrat.regNum, regStrat.regNum);
        //    RegVar address;
        //    ASM::mov_r_static_var_address(address.regNum, relAddress);
        //    ASM::mov_a_r(address.regNum, regStrat.regNum);
        //}
        StaticStrat(const RegVar& init) : relAddress(Allocate())
        {
            RegVar address;
            ASM::mov_r_static_var_address(address.regNum, relAddress);
            ASM::mov_a_r(address.regNum, init.regNum);
        }
        operator RegVar() const
        {
            RegVar result;
            ASM::mov_r_static_var_address(result.regNum, relAddress);
            ASM::mov_r_a(result.regNum, result.regNum);
            return result;
        }
        template <class T>
        void Set(const T& value)
        {
            const RegVar& regVar(value);
            RegVar address;
            ASM::mov_r_static_var_address(address.regNum, relAddress);
            ASM::mov_a_r(address.regNum, regVar.regNum);
        }
    };
    
    template <class StoringStrat>
    class Var
    {
        StoringStrat storingStrat;
    public:
        Var() { }
        
        Var(const Var<StoringStrat>& other) :
        storingStrat((const RegVar&)other.storingStrat)
        { }
        
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










