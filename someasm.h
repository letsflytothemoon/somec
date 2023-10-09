#pragma once
#include <vector>
#include <stack>
#include <iostream>
#include <string>
#include <cmath>

template <class C_OPTS>
class SomeASM
{
  static std::string to_string(int i)
  {
    return std::to_string(i);
  }
  
  struct Instuction
  {
    virtual int  size() = 0;
    virtual void asm_text() = 0;
    virtual void code(int&) = 0;
    virtual int bytes(int) = 0;
    virtual ~Instuction() {}
  };
  
  template <class Instr, class Dummy = void>
  struct InstrInfo
  {
    static int& counter()
    {
      static int value = 0;
      return value;
    }
    static int& opcode()
    {
      static int value;
      return value;
    }
  };
  
  struct Image
  {
    std::vector<Instuction*> instructions;
    std::vector<int>          label_address;
    int size;
    int static_vars_counter;
    int instructions_types_counter;
    int LLO, LHI, RLO, RHI;
    int OP_BITS;
    
    Image() : size(0), static_vars_counter(0) {}
    ~Image()
    {
      OP_BITS = log2(instructions_types_counter - 1) + 1;
      int reg_address_bits = log2(C_OPTS::REGS - 1) + 1;
      LLO = OP_BITS;
      LHI = LLO + reg_address_bits - 1;
      RLO = LHI + 1;
      RHI = RLO + reg_address_bits - 1;
      compile_asm();
      compile_verilog();
      compile_code();
    }
    
    template <class Instr, class ... ARGS>
    void push_instruction(ARGS&& ... args)
    {
      Instuction* instruction = new Instr(args ...);
      size += instruction->size();
      instructions.push_back(instruction);
      if(InstrInfo<Instr>::counter()++ == 0)
        InstrInfo<Instr>::opcode() = instructions_types_counter++;
    }
    
    int get_static_var_address(int _rel_address)
    {
      return size + _rel_address;
    }
    
    void compile_asm()
    {
      for(unsigned int i = 0; i < instructions.size(); i++)
        instructions[i]->asm_text();
      C_OPTS::asm_ostr() << std::endl;
    }
    
    void compile_verilog()
    {
      C_OPTS::verilog_ostr() << "module CPU(clock_i," << std::endl;
      C_OPTS::verilog_ostr() << "           reset_i," << std::endl;
      C_OPTS::verilog_ostr() << "           abus_o," << std::endl;
      C_OPTS::verilog_ostr() << "           dbus_o," << std::endl;
      C_OPTS::verilog_ostr() << "           dbus_i," << std::endl;
      C_OPTS::verilog_ostr() << "           wren_o," << std::endl;
      C_OPTS::verilog_ostr() << "           ioabus_o," << std::endl;
      C_OPTS::verilog_ostr() << "           iodbus_o," << std::endl;
      C_OPTS::verilog_ostr() << "           iodbus_i," << std::endl;
      C_OPTS::verilog_ostr() << "           iowren_o," << std::endl;
      C_OPTS::verilog_ostr() << "           iorden_o);" << std::endl;
      C_OPTS::verilog_ostr() << "  " << std::endl;
      C_OPTS::verilog_ostr() << "  localparam W   = " << to_string(C_OPTS::BITS) << ";" << std::endl;
      C_OPTS::verilog_ostr() << "  localparam CHI = " << to_string(image.OP_BITS - 1) << ";" << std::endl;
      C_OPTS::verilog_ostr() << "  localparam CLO = " << to_string(0) << ";" << std::endl;
      C_OPTS::verilog_ostr() << "  localparam LHI = " << to_string(image.LHI) << ";" << std::endl;
      C_OPTS::verilog_ostr() << "  localparam LLO = " << to_string(image.LLO) << ";" << std::endl;
      C_OPTS::verilog_ostr() << "  localparam RHI = " << to_string(image.RHI) << ";" << std::endl;
      C_OPTS::verilog_ostr() << "  localparam RLO = " << to_string(image.RLO) << ";" << std::endl;
      C_OPTS::verilog_ostr() << "  localparam R   = " << to_string(C_OPTS::REGS) << ";" << std::endl;
      C_OPTS::verilog_ostr() << "  localparam DSP_INITIAL = " << to_string(C_OPTS::RAMSIZE - 1) << ";" << std::endl;
      C_OPTS::verilog_ostr() << "  localparam ASP_INITIAL = " << to_string(image.size + static_vars_counter) << ";" << std::endl;
      C_OPTS::verilog_ostr() << "  " << std::endl;
      C_OPTS::verilog_ostr() << "  input  wire clock_i;" << std::endl;
      C_OPTS::verilog_ostr() << "  input  wire reset_i;" << std::endl;
      C_OPTS::verilog_ostr() << "  output reg  [W-1 : 0] abus_o;" << std::endl;
      C_OPTS::verilog_ostr() << "  output reg  [W-1 : 0] dbus_o;" << std::endl;
      C_OPTS::verilog_ostr() << "  input  wire [W-1 : 0] dbus_i;" << std::endl;
      C_OPTS::verilog_ostr() << "  output wire wren_o;" << std::endl;
      C_OPTS::verilog_ostr() << "  output reg  [W-1 : 0] ioabus_o;" << std::endl;
      C_OPTS::verilog_ostr() << "  output reg  [W-1 : 0] iodbus_o;" << std::endl;
      C_OPTS::verilog_ostr() << "  input  wire [W-1 : 0] iodbus_i;" << std::endl;
      C_OPTS::verilog_ostr() << "  output reg  iowren_o;" << std::endl;
      C_OPTS::verilog_ostr() << "  output reg  iorden_o;" << std::endl;
      C_OPTS::verilog_ostr() << "  reg  wren;" << std::endl;
      C_OPTS::verilog_ostr() << "  assign wren_o = (abus_o < " << to_string(image.size);
      C_OPTS::verilog_ostr() << ") ? 1'b0 : (wren & ~reset_i);" << std::endl;
      C_OPTS::verilog_ostr() << "  " << std::endl;
      C_OPTS::verilog_ostr() << "  reg  [W-1 : 0] dsp;" << std::endl;
      C_OPTS::verilog_ostr() << "  reg  [W-1 : 0] asp;" << std::endl;
      C_OPTS::verilog_ostr() << "  " << std::endl;
      C_OPTS::verilog_ostr() << "  reg  [R-1 : 0][W-1 : 0] regs;" << std::endl;
      C_OPTS::verilog_ostr() << "  reg  [W-1 : 0] cp;" << std::endl;
      C_OPTS::verilog_ostr() << "  reg z_f;//zero" << std::endl;
      C_OPTS::verilog_ostr() << "  reg c_f;//carry" << std::endl;
      C_OPTS::verilog_ostr() << "  reg o_f;//overflow" << std::endl;
      C_OPTS::verilog_ostr() << "  reg l_f;//less" << std::endl;
      C_OPTS::verilog_ostr() << "  reg g_f;//greather" << std::endl;
      C_OPTS::verilog_ostr() << "  " << std::endl;
      C_OPTS::verilog_ostr() << "  reg  [  5 : 0] stage;" << std::endl;
      C_OPTS::verilog_ostr() << "  reg  [W-1 : 0] command_latch;" << std::endl;
      C_OPTS::verilog_ostr() << "  wire [W-1 : 0] command = (stage == 6'd0) ? dbus_i : command_latch;" << std::endl;
      C_OPTS::verilog_ostr() << "" << std::endl;
      C_OPTS::verilog_ostr() << "  wire [W : 0] add_result = regs[command[LHI:LLO]] + regs[command[RHI:RLO]];" << std::endl;
      C_OPTS::verilog_ostr() << "  wire add_c_f = add_result[W];" << std::endl;
      C_OPTS::verilog_ostr() << "  wire add_z_f = ~|add_result[W-1 : 0];" << std::endl;
      C_OPTS::verilog_ostr() << "" << std::endl;
      C_OPTS::verilog_ostr() << "  wire [W-1 : 0] sub_result = regs[command[LHI:LLO]] - regs[command[RHI:RLO]];" << std::endl;
      C_OPTS::verilog_ostr() << "  wire sub_z_f = ~|sub_result[W-1 : 0];" << std::endl;
      C_OPTS::verilog_ostr() << "" << std::endl;
      C_OPTS::verilog_ostr() << "  wire [2*W-1 : 0] mul_result = regs[command[LHI:LLO]] * regs[command[RHI:RLO]];" << std::endl;
      C_OPTS::verilog_ostr() << "  wire mul_o_f = |mul_result[2*W-1 : W];" << std::endl;
      C_OPTS::verilog_ostr() << "" << std::endl;
      C_OPTS::verilog_ostr() << "  reg  divider_push;" << std::endl;
      C_OPTS::verilog_ostr() << "  reg  [W-1 : 0] divider_done_sr;" << std::endl;
      C_OPTS::verilog_ostr() << "  wire divider_done = divider_done_sr[W-1];" << std::endl;
      C_OPTS::verilog_ostr() << "  reg  [W * 2 - 2 : 0] divider_l;" << std::endl;
      C_OPTS::verilog_ostr() << "  reg  [W * 2 - 2 : 0] divider_r;" << std::endl;
      C_OPTS::verilog_ostr() << "  reg  [W-1 : 0] div_result;" << std::endl;
      C_OPTS::verilog_ostr() << "  wire [W-1 : 0] mod_result = divider_l[W-1 : 0];" << std::endl;
      C_OPTS::verilog_ostr() << "" << std::endl;
      C_OPTS::verilog_ostr() << "  wire divider_bit_value = (divider_l < divider_r) ? 1'b0 : 1'b1;" << std::endl;
      C_OPTS::verilog_ostr() << "" << std::endl;
      C_OPTS::verilog_ostr() << "  always_ff@(posedge clock_i)" << std::endl;
      C_OPTS::verilog_ostr() << "    if(divider_push)" << std::endl;
      C_OPTS::verilog_ostr() << "    begin" << std::endl;
      C_OPTS::verilog_ostr() << "      divider_l <= {{(W-1){1'b0}}, regs[command[LHI:LLO]]};" << std::endl;
      C_OPTS::verilog_ostr() << "      divider_r <= {regs[command[RHI:RLO]]};" << std::endl;
      C_OPTS::verilog_ostr() << "      divider_done_sr <= 'b0;" << std::endl;
      C_OPTS::verilog_ostr() << "    end" << std::endl;
      C_OPTS::verilog_ostr() << "    else" << std::endl;
      C_OPTS::verilog_ostr() << "      if(~divider_done)" << std::endl;
      C_OPTS::verilog_ostr() << "      begin" << std::endl;
      C_OPTS::verilog_ostr() << "        div_result <= {div_result[W-2 : 0], divider_bit_value};" << std::endl;
      C_OPTS::verilog_ostr() << "        divider_l  <= divider_bit_value ? divider_l - divider_r : divider_l;" << std::endl;
      C_OPTS::verilog_ostr() << "        divider_r  <= {1'b0, divider_r[W*2-2 : 1]};" << std::endl;
      C_OPTS::verilog_ostr() << "        divider_done_sr <= {divider_done_sr[W-2 : 0], 1'b1};" << std::endl;
      C_OPTS::verilog_ostr() << "      end" << std::endl;
      C_OPTS::verilog_ostr() << "" << std::endl;      
      C_OPTS::verilog_ostr() << "  wire cmp_l_f = (regs[command[LHI:LLO]] <  regs[command[RHI:RLO]]) ? 1'b1 : 1'b0;" << std::endl;
      C_OPTS::verilog_ostr() << "  wire cmp_g_f = (regs[command[LHI:LLO]] >  regs[command[RHI:RLO]]) ? 1'b1 : 1'b0;" << std::endl;
      C_OPTS::verilog_ostr() << "  wire cmp_z_f = (regs[command[LHI:LLO]] == regs[command[RHI:RLO]]) ? 1'b1 : 1'b0;" << std::endl;
      C_OPTS::verilog_ostr() << "" << std::endl;
      C_OPTS::verilog_ostr() << "  wire [W-1 : 0] or_result = regs[command[LHI:LLO]] | regs[command[RHI:RLO]];" << std::endl;
      C_OPTS::verilog_ostr() << "  wire or_z_f = ~|or_result;" << std::endl;
      C_OPTS::verilog_ostr() << "" << std::endl;
      C_OPTS::verilog_ostr() << "  wire [W-1 : 0] and_result = regs[command[LHI:LLO]] & regs[command[RHI:RLO]];" << std::endl;
      C_OPTS::verilog_ostr() << "  wire and_z_f = ~|and_result;" << std::endl;
      C_OPTS::verilog_ostr() << "" << std::endl;
      C_OPTS::verilog_ostr() << "  wire [W-1 : 0] xor_result = regs[command[LHI:LLO]] ^ regs[command[RHI:RLO]];" << std::endl;
      C_OPTS::verilog_ostr() << "  wire xor_z_f = ~|xor_result;" << std::endl;
      C_OPTS::verilog_ostr() << "" << std::endl;
      C_OPTS::verilog_ostr() << "  wire [W+1 : 0] shifts = {1'b0, regs[command[LHI:LLO]], 1'b0};" << std::endl;
      C_OPTS::verilog_ostr() << "  wire [W-1 : 0] ls_result = shifts[W+1 : 2];" << std::endl;
      C_OPTS::verilog_ostr() << "  wire ls_c_f = shifts[1];" << std::endl;
      C_OPTS::verilog_ostr() << "  wire ls_z_f = ~|ls_result;" << std::endl;
      C_OPTS::verilog_ostr() << "  wire [W-1 : 0] rs_result = shifts[W-1 : 0];" << std::endl;
      C_OPTS::verilog_ostr() << "  reg  [W-1 : 0] shift_counter;" << std::endl;
      C_OPTS::verilog_ostr() << "  wire rs_c_f = shifts[W];" << std::endl;
      C_OPTS::verilog_ostr() << "  wire rs_z_f = ~|rs_result;" << std::endl;
      C_OPTS::verilog_ostr() << "  " << std::endl;
      C_OPTS::verilog_ostr() << "  wire [W : 0] inc_result = regs[command[LHI:LLO]] + 1'b1;" << std::endl;
      C_OPTS::verilog_ostr() << "  wire inc_z_f = ~|inc_result[W-1 : 0];" << std::endl;
      C_OPTS::verilog_ostr() << "  wire inc_c_f = inc_result[W];" << std::endl;
      C_OPTS::verilog_ostr() << "  " << std::endl;
      C_OPTS::verilog_ostr() << "  wire [W-1 : 0] dec_result = regs[command[LHI:LLO]] - 1'b1;" << std::endl;
      C_OPTS::verilog_ostr() << "  wire dec_z_f = ~|dec_result;" << std::endl;
      C_OPTS::verilog_ostr() << "  wire dec_c_f = ~|regs[command[LHI:LLO]];" << std::endl;
      C_OPTS::verilog_ostr() << "  " << std::endl;
      C_OPTS::verilog_ostr() << "  always_ff@(posedge clock_i)" << std::endl;
      C_OPTS::verilog_ostr() << "  if(reset_i)" << std::endl;
      C_OPTS::verilog_ostr() << "  begin" << std::endl;
      C_OPTS::verilog_ostr() << "    divider_push <= 1'b0;" << std::endl;
      C_OPTS::verilog_ostr() << "    stage        <= 'b0;" << std::endl;
      C_OPTS::verilog_ostr() << "    cp           <= 'b0;" << std::endl;
      C_OPTS::verilog_ostr() << "    dsp          <= DSP_INITIAL;" << std::endl;
      C_OPTS::verilog_ostr() << "    asp          <= ASP_INITIAL;" << std::endl;
      C_OPTS::verilog_ostr() << "    abus_o       <= 'b0;" << std::endl;
      C_OPTS::verilog_ostr() << "    wren         <= 1'b0;" << std::endl;
      C_OPTS::verilog_ostr() << "    iowren_o     <= 1'b0;" << std::endl;
      C_OPTS::verilog_ostr() << "    iorden_o     <= 1'b0;" << std::endl;
      C_OPTS::verilog_ostr() << "  end" << std::endl;
      C_OPTS::verilog_ostr() << "  else" << std::endl;
      C_OPTS::verilog_ostr() << "  begin" << std::endl;
      C_OPTS::verilog_ostr() << "    if(~|stage)" << std::endl;
      C_OPTS::verilog_ostr() << "      command_latch <= dbus_i;" << std::endl;
      C_OPTS::verilog_ostr() << "    case(command[CHI : CLO])" << std::endl;
      if(InstrInfo<MOV_R_R>::counter())
      {
        C_OPTS::verilog_ostr() << "      " << to_string(image.OP_BITS) << "'d";
        C_OPTS::verilog_ostr() << to_string(InstrInfo<MOV_R_R>::opcode()) << ": //mov_r_r" << std::endl;
        C_OPTS::verilog_ostr() << "      begin" << std::endl;
        C_OPTS::verilog_ostr() << "        regs[command[LHI:LLO]] <= regs[command[RHI:RLO]];" << std::endl;
        C_OPTS::verilog_ostr() << "        cp     <= cp + 1'b1;" << std::endl;
        C_OPTS::verilog_ostr() << "        abus_o <= abus_o + 1'b1;" << std::endl;
        C_OPTS::verilog_ostr() << "      end" << std::endl;
      }
      if(InstrInfo<MOV_R_A>::counter())
      {
        C_OPTS::verilog_ostr() << "      " << to_string(image.OP_BITS) << "'d";
        C_OPTS::verilog_ostr() << to_string(InstrInfo<MOV_R_A>::opcode()) << ": //mov_r_a" << std::endl;
        C_OPTS::verilog_ostr() << "      begin" << std::endl;
        C_OPTS::verilog_ostr() << "        case(stage)" << std::endl;
        C_OPTS::verilog_ostr() << "          6'd0:" << std::endl;
        C_OPTS::verilog_ostr() << "          begin" << std::endl;
        C_OPTS::verilog_ostr() << "            abus_o <= regs[command[RHI:RLO]];" << std::endl;
        C_OPTS::verilog_ostr() << "            stage  <= stage + 1'b1;" << std::endl;
        C_OPTS::verilog_ostr() << "          end" << std::endl;
        C_OPTS::verilog_ostr() << "          6'd1:" << std::endl;
        C_OPTS::verilog_ostr() << "          begin" << std::endl;
        C_OPTS::verilog_ostr() << "            regs[command[LHI:LLO]] <= dbus_i;" << std::endl;
        C_OPTS::verilog_ostr() << "            cp     <= cp + 1'b1;" << std::endl;
        C_OPTS::verilog_ostr() << "            abus_o <= cp + 1'b1;" << std::endl;
        C_OPTS::verilog_ostr() << "            stage  <= 'b0;" << std::endl;
        C_OPTS::verilog_ostr() << "          end" << std::endl;
        C_OPTS::verilog_ostr() << "        endcase" << std::endl;
        C_OPTS::verilog_ostr() << "      end" << std::endl;
      }
      if(InstrInfo<MOV_A_R>::counter())
      {
        C_OPTS::verilog_ostr() << "      " << to_string(image.OP_BITS) << "'d";
        C_OPTS::verilog_ostr() << to_string(InstrInfo<MOV_A_R>::opcode()) << ": //mov_a_r" << std::endl;
        C_OPTS::verilog_ostr() << "      begin" << std::endl;
        C_OPTS::verilog_ostr() << "        case(stage)" << std::endl;
        C_OPTS::verilog_ostr() << "          6'd0:" << std::endl;
        C_OPTS::verilog_ostr() << "          begin" << std::endl;
        C_OPTS::verilog_ostr() << "            abus_o <= regs[command[LHI:LLO]];" << std::endl;
        C_OPTS::verilog_ostr() << "            dbus_o <= regs[command[RHI:RLO]];" << std::endl;
        C_OPTS::verilog_ostr() << "            wren   <= 1'b1;" << std::endl;
        C_OPTS::verilog_ostr() << "            stage  <= stage + 1'b1;" << std::endl;
        C_OPTS::verilog_ostr() << "          end" << std::endl;
        C_OPTS::verilog_ostr() << "          6'd1:" << std::endl;
        C_OPTS::verilog_ostr() << "          begin" << std::endl;
        C_OPTS::verilog_ostr() << "            wren   <= 1'b0;" << std::endl;
        C_OPTS::verilog_ostr() << "            stage  <= 'b0;" << std::endl;
        C_OPTS::verilog_ostr() << "            cp     <= cp + 1'b1;" << std::endl;
        C_OPTS::verilog_ostr() << "            abus_o <= cp + 1'b1;" << std::endl;
        C_OPTS::verilog_ostr() << "          end" << std::endl;
        C_OPTS::verilog_ostr() << "        endcase" << std::endl;
        C_OPTS::verilog_ostr() << "      end" << std::endl;
      }
      if(InstrInfo<ADD_R_R>::counter())
      {
        C_OPTS::verilog_ostr() << "      " << to_string(image.OP_BITS) << "'d";
        C_OPTS::verilog_ostr() << to_string(InstrInfo<ADD_R_R>::opcode()) << ": //add_r_r" << std::endl;
        C_OPTS::verilog_ostr() << "      begin" << std::endl;
        C_OPTS::verilog_ostr() << "        regs[command[LHI:LLO]] <= add_result;" << std::endl;
        C_OPTS::verilog_ostr() << "        c_f    <= add_c_f;" << std::endl;
        C_OPTS::verilog_ostr() << "        z_f    <= add_z_f;" << std::endl;
        C_OPTS::verilog_ostr() << "        cp     <= cp + 1'b1;" << std::endl;
        C_OPTS::verilog_ostr() << "        abus_o <= cp + 1'b1;" << std::endl;
        C_OPTS::verilog_ostr() << "      end" << std::endl;
      }
      if(InstrInfo<SUB_R_R>::counter())
      {
        C_OPTS::verilog_ostr() << "      " << to_string(image.OP_BITS) << "'d";
        C_OPTS::verilog_ostr() << to_string(InstrInfo<SUB_R_R>::opcode()) << ": //sub_r_r" << std::endl;
        C_OPTS::verilog_ostr() << "      begin" << std::endl;
        C_OPTS::verilog_ostr() << "        regs[command[LHI:LLO]] <= sub_result;" << std::endl;
        C_OPTS::verilog_ostr() << "        z_f    <= sub_z_f;" << std::endl;
        C_OPTS::verilog_ostr() << "        cp     <= cp + 1'b1;" << std::endl;
        C_OPTS::verilog_ostr() << "        abus_o <= cp + 1'b1;" << std::endl;
        C_OPTS::verilog_ostr() << "      end" << std::endl;
      }
      if(InstrInfo<MUL_R_R>::counter())
      {
        C_OPTS::verilog_ostr() << "      " << to_string(image.OP_BITS) << "'d";
        C_OPTS::verilog_ostr() << to_string(InstrInfo<MUL_R_R>::opcode()) << ": //mul_r_r" << std::endl;
        C_OPTS::verilog_ostr() << "      begin" << std::endl;
        C_OPTS::verilog_ostr() << "        regs[command[LHI:LLO]] <= mul_result;" << std::endl;
        C_OPTS::verilog_ostr() << "        o_f    <= mul_o_f;" << std::endl;
        C_OPTS::verilog_ostr() << "        cp     <= cp + 1'b1;" << std::endl;
        C_OPTS::verilog_ostr() << "        abus_o <= cp + 1'b1;" << std::endl;
        C_OPTS::verilog_ostr() << "      end" << std::endl;
      }
      if(InstrInfo<DIV_R_R>::counter())
      {
        C_OPTS::verilog_ostr() << "      " << to_string(image.OP_BITS) << "'d";
        C_OPTS::verilog_ostr() << to_string(InstrInfo<DIV_R_R>::opcode()) << ": //div_r_r" << std::endl;
        C_OPTS::verilog_ostr() << "      begin" << std::endl;
        C_OPTS::verilog_ostr() << "        case(stage)" << std::endl;
        C_OPTS::verilog_ostr() << "          6'd0:" << std::endl;
        C_OPTS::verilog_ostr() << "          begin" << std::endl;
        C_OPTS::verilog_ostr() << "            divider_push <= 1'b1;" << std::endl;
        C_OPTS::verilog_ostr() << "            stage        <= stage + 1'b1;" << std::endl;
        C_OPTS::verilog_ostr() << "          end" << std::endl;
        C_OPTS::verilog_ostr() << "          6'd1:" << std::endl;
        C_OPTS::verilog_ostr() << "          begin" << std::endl;
        C_OPTS::verilog_ostr() << "            divider_push <= 1'b0;" << std::endl;
        C_OPTS::verilog_ostr() << "            stage        <= stage + 1'b1;" << std::endl;
        C_OPTS::verilog_ostr() << "          end" << std::endl;
        C_OPTS::verilog_ostr() << "          6'd2:" << std::endl;
        C_OPTS::verilog_ostr() << "          begin" << std::endl;
        C_OPTS::verilog_ostr() << "            if(divider_done)" << std::endl;
        C_OPTS::verilog_ostr() << "            begin" << std::endl;
        C_OPTS::verilog_ostr() << "              regs[command[LHI:LLO]] <= div_result;" << std::endl;
        C_OPTS::verilog_ostr() << "              cp     <= cp + 1'b1;" << std::endl;
        C_OPTS::verilog_ostr() << "              abus_o <= cp + 1'b1;" << std::endl;
        C_OPTS::verilog_ostr() << "              stage  <= 'b0;" << std::endl;
        C_OPTS::verilog_ostr() << "            end" << std::endl;
        C_OPTS::verilog_ostr() << "          end" << std::endl;
        C_OPTS::verilog_ostr() << "        endcase" << std::endl;
        C_OPTS::verilog_ostr() << "      end" << std::endl;
      }
      if(InstrInfo<MOD_R_R>::counter())
      {
        C_OPTS::verilog_ostr() << "      " << to_string(image.OP_BITS) << "'d";
        C_OPTS::verilog_ostr() << to_string(InstrInfo<MOD_R_R>::opcode()) << ": //mod_r_r" << std::endl;
        C_OPTS::verilog_ostr() << "      begin" << std::endl;
        C_OPTS::verilog_ostr() << "        case(stage)" << std::endl;
        C_OPTS::verilog_ostr() << "          6'd0:" << std::endl;
        C_OPTS::verilog_ostr() << "          begin" << std::endl;
        C_OPTS::verilog_ostr() << "            divider_push <= 1'b1;" << std::endl;
        C_OPTS::verilog_ostr() << "            stage        <= stage + 1'b1;" << std::endl;
        C_OPTS::verilog_ostr() << "          end" << std::endl;
        C_OPTS::verilog_ostr() << "          6'd1:" << std::endl;
        C_OPTS::verilog_ostr() << "          begin" << std::endl;
        C_OPTS::verilog_ostr() << "            divider_push <= 1'b0;" << std::endl;
        C_OPTS::verilog_ostr() << "            stage        <= stage + 1'b1;" << std::endl;
        C_OPTS::verilog_ostr() << "          end" << std::endl;
        C_OPTS::verilog_ostr() << "          6'd2:" << std::endl;
        C_OPTS::verilog_ostr() << "          begin" << std::endl;
        C_OPTS::verilog_ostr() << "            if(divider_done)" << std::endl;
        C_OPTS::verilog_ostr() << "            begin" << std::endl;
        C_OPTS::verilog_ostr() << "              regs[command[LHI:LLO]] <= mod_result;" << std::endl;
        C_OPTS::verilog_ostr() << "              cp     <= cp + 1'b1;" << std::endl;
        C_OPTS::verilog_ostr() << "              abus_o <= cp + 1'b1;" << std::endl;
        C_OPTS::verilog_ostr() << "              stage  <= 'b0;" << std::endl;
        C_OPTS::verilog_ostr() << "            end" << std::endl;
        C_OPTS::verilog_ostr() << "          end" << std::endl;
        C_OPTS::verilog_ostr() << "        endcase" << std::endl;
        C_OPTS::verilog_ostr() << "      end" << std::endl;
      }
      if(InstrInfo<CMP_R_R>::counter())
      {
        C_OPTS::verilog_ostr() << "      " << to_string(image.OP_BITS) << "'d";
        C_OPTS::verilog_ostr() << to_string(InstrInfo<CMP_R_R>::opcode()) << ": //cmp_r_r" << std::endl;
        C_OPTS::verilog_ostr() << "      begin" << std::endl;
        C_OPTS::verilog_ostr() << "        l_f    <= cmp_l_f;" << std::endl;
        C_OPTS::verilog_ostr() << "        g_f    <= cmp_g_f;" << std::endl;
        C_OPTS::verilog_ostr() << "        z_f    <= cmp_z_f;" << std::endl;
        C_OPTS::verilog_ostr() << "        cp     <= cp + 1'b1;" << std::endl;
        C_OPTS::verilog_ostr() << "        abus_o <= cp + 1'b1;" << std::endl;
        C_OPTS::verilog_ostr() << "      end" << std::endl;
      }
      if(InstrInfo<OR_R_R>::counter())
      {
        C_OPTS::verilog_ostr() << "      " << to_string(image.OP_BITS) << "'d";
        C_OPTS::verilog_ostr() << to_string(InstrInfo<OR_R_R>::opcode()) << ": //or_r_r" << std::endl;
        C_OPTS::verilog_ostr() << "      begin" << std::endl;
        C_OPTS::verilog_ostr() << "        regs[command[LHI:LLO]] <= or_result;" << std::endl;
        C_OPTS::verilog_ostr() << "        z_f    <= or_z_f;" << std::endl;
        C_OPTS::verilog_ostr() << "        cp     <= cp + 1'b1;" << std::endl;
        C_OPTS::verilog_ostr() << "        abus_o <= cp + 1'b1;" << std::endl;
        C_OPTS::verilog_ostr() << "      end" << std::endl;
      }
      if(InstrInfo<AND_R_R>::counter())
      {
        C_OPTS::verilog_ostr() << "      " << to_string(image.OP_BITS) << "'d";
        C_OPTS::verilog_ostr() << to_string(InstrInfo<AND_R_R>::opcode()) << ": //and_r_r" << std::endl;
        C_OPTS::verilog_ostr() << "      begin" << std::endl;
        C_OPTS::verilog_ostr() << "        regs[command[LHI:LLO]] <= and_result;" << std::endl;
        C_OPTS::verilog_ostr() << "        z_f    <= and_z_f;" << std::endl;
        C_OPTS::verilog_ostr() << "        cp     <= cp + 1'b1;" << std::endl;
        C_OPTS::verilog_ostr() << "        abus_o <= cp + 1'b1;" << std::endl;
        C_OPTS::verilog_ostr() << "      end" << std::endl;
      }
      if(InstrInfo<XOR_R_R>::counter())
      {
        C_OPTS::verilog_ostr() << "      " << to_string(image.OP_BITS) << "'d";
        C_OPTS::verilog_ostr() << to_string(InstrInfo<XOR_R_R>::opcode()) << ": //xor_r_r" << std::endl;
        C_OPTS::verilog_ostr() << "      begin" << std::endl;
        C_OPTS::verilog_ostr() << "        regs[command[LHI:LLO]] <= xor_result;" << std::endl;
        C_OPTS::verilog_ostr() << "        z_f    <= xor_z_f;" << std::endl;
        C_OPTS::verilog_ostr() << "        cp     <= cp + 1'b1;" << std::endl;
        C_OPTS::verilog_ostr() << "        abus_o <= cp + 1'b1;" << std::endl;
        C_OPTS::verilog_ostr() << "      end" << std::endl;
      }
      if(InstrInfo<IN_R_R>::counter())
      {
        C_OPTS::verilog_ostr() << "      " << to_string(image.OP_BITS) << "'d";
        C_OPTS::verilog_ostr() << to_string(InstrInfo<IN_R_R>::opcode()) << ": //in_r_r" << std::endl;
        C_OPTS::verilog_ostr() << "      begin" << std::endl;
        C_OPTS::verilog_ostr() << "        case(stage)" << std::endl;
        C_OPTS::verilog_ostr() << "          6'd0:" << std::endl;
        C_OPTS::verilog_ostr() << "          begin" << std::endl;
        C_OPTS::verilog_ostr() << "            ioabus_o <= regs[command[RHI:RLO]];" << std::endl;
        C_OPTS::verilog_ostr() << "            iorden_o <= 1'b1;" << std::endl;
        C_OPTS::verilog_ostr() << "            stage    <= stage + 1'b1;" << std::endl;
        C_OPTS::verilog_ostr() << "          end" << std::endl;
        C_OPTS::verilog_ostr() << "          6'd1:" << std::endl;
        C_OPTS::verilog_ostr() << "          begin" << std::endl;
        C_OPTS::verilog_ostr() << "            regs[command[LHI:LLO]] <= iodbus_i;" << std::endl;
        C_OPTS::verilog_ostr() << "            iorden_o <= 1'b0;" << std::endl;
        C_OPTS::verilog_ostr() << "            stage    <= 'b0;" << std::endl;
        C_OPTS::verilog_ostr() << "            cp       <= cp + 1'b1;" << std::endl;
        C_OPTS::verilog_ostr() << "            abus_o   <= cp + 1'b1;" << std::endl;
        C_OPTS::verilog_ostr() << "          end" << std::endl;
        C_OPTS::verilog_ostr() << "        endcase" << std::endl;
        C_OPTS::verilog_ostr() << "      end" << std::endl;
      }
      if(InstrInfo<OUT_R_R>::counter())
      {
        C_OPTS::verilog_ostr() << "      " << to_string(image.OP_BITS) << "'d";
        C_OPTS::verilog_ostr() << to_string(InstrInfo<OUT_R_R>::opcode()) << ": //out_r_r" << std::endl;
        C_OPTS::verilog_ostr() << "      begin" << std::endl;
        C_OPTS::verilog_ostr() << "        case(stage)" << std::endl;
        C_OPTS::verilog_ostr() << "          6'd0:" << std::endl;
        C_OPTS::verilog_ostr() << "          begin" << std::endl;
        C_OPTS::verilog_ostr() << "            ioabus_o <= regs[command[LHI:LLO]];" << std::endl;
        C_OPTS::verilog_ostr() << "            iodbus_o <= regs[command[RHI:RLO]];" << std::endl;
        C_OPTS::verilog_ostr() << "            iowren_o <= 1'b1;" << std::endl;
        C_OPTS::verilog_ostr() << "            stage    <= stage + 1'b1;" << std::endl;
        C_OPTS::verilog_ostr() << "          end" << std::endl;
        C_OPTS::verilog_ostr() << "          6'd1:" << std::endl;
        C_OPTS::verilog_ostr() << "          begin" << std::endl;
        C_OPTS::verilog_ostr() << "            iowren_o <= 1'b0;" << std::endl;
        C_OPTS::verilog_ostr() << "            stage    <= 'b0;" << std::endl;
        C_OPTS::verilog_ostr() << "            cp       <= cp + 1'b1;" << std::endl;
        C_OPTS::verilog_ostr() << "            abus_o   <= cp + 1'b1;" << std::endl;
        C_OPTS::verilog_ostr() << "          end" << std::endl;
        C_OPTS::verilog_ostr() << "        endcase" << std::endl;
        C_OPTS::verilog_ostr() << "      end" << std::endl;
      }
      if(InstrInfo<MOV_R_B>::counter())
      {
        C_OPTS::verilog_ostr() << "      " << to_string(image.OP_BITS) << "'d";
        C_OPTS::verilog_ostr() << to_string(InstrInfo<MOV_R_B>::opcode()) << ": //mov_r_b" << std::endl;
        C_OPTS::verilog_ostr() << "      begin" << std::endl;
        C_OPTS::verilog_ostr() << "        case(stage)" << std::endl;
        C_OPTS::verilog_ostr() << "          6'd0:" << std::endl;
        C_OPTS::verilog_ostr() << "          begin" << std::endl;
        C_OPTS::verilog_ostr() << "            abus_o <= abus_o + 1'b1;" << std::endl;
        C_OPTS::verilog_ostr() << "            stage  <= stage + 1'b1;" << std::endl;
        C_OPTS::verilog_ostr() << "          end" << std::endl;
        C_OPTS::verilog_ostr() << "          6'd1:" << std::endl;
        C_OPTS::verilog_ostr() << "          begin" << std::endl;
        C_OPTS::verilog_ostr() << "            regs[command[LHI:LLO]] <= dbus_i;" << std::endl;
        C_OPTS::verilog_ostr() << "            cp     <= cp + 2'd2;" << std::endl;
        C_OPTS::verilog_ostr() << "            abus_o <= cp + 2'd2;" << std::endl;
        C_OPTS::verilog_ostr() << "            stage  <= 'b0;" << std::endl;
        C_OPTS::verilog_ostr() << "          end" << std::endl;
        C_OPTS::verilog_ostr() << "        endcase" << std::endl;
        C_OPTS::verilog_ostr() << "      end" << std::endl;
      }
      if(InstrInfo<MOV_R_S>::counter())
      {
        C_OPTS::verilog_ostr() << "      " << to_string(image.OP_BITS) << "'d";
        C_OPTS::verilog_ostr() << to_string(InstrInfo<MOV_R_S>::opcode()) << ": //mov_r_s" << std::endl;
        C_OPTS::verilog_ostr() << "      begin" << std::endl;
        C_OPTS::verilog_ostr() << "        case(stage)" << std::endl;
        C_OPTS::verilog_ostr() << "          6'd0:" << std::endl;
        C_OPTS::verilog_ostr() << "          begin" << std::endl;
        C_OPTS::verilog_ostr() << "            abus_o <= abus_o + 1'b1;" << std::endl;
        C_OPTS::verilog_ostr() << "            stage  <= stage + 1'b1;" << std::endl;
        C_OPTS::verilog_ostr() << "          end" << std::endl;
        C_OPTS::verilog_ostr() << "          6'd1:" << std::endl;
        C_OPTS::verilog_ostr() << "          begin" << std::endl;
        C_OPTS::verilog_ostr() << "            abus_o <= dsp + dbus_i;" << std::endl;
        C_OPTS::verilog_ostr() << "            stage <= stage + 1'b1;" << std::endl;
        C_OPTS::verilog_ostr() << "          end" << std::endl;
        C_OPTS::verilog_ostr() << "          6'd2:" << std::endl;
        C_OPTS::verilog_ostr() << "          begin" << std::endl;
        C_OPTS::verilog_ostr() << "            regs[command[LHI:LLO]] <= dbus_i;" << std::endl;
        C_OPTS::verilog_ostr() << "            cp     <= cp + 2'd2;" << std::endl;
        C_OPTS::verilog_ostr() << "            abus_o <= cp + 2'd2;" << std::endl;
        C_OPTS::verilog_ostr() << "            stage  <= 'b0;" << std::endl;
        C_OPTS::verilog_ostr() << "          end" << std::endl;
        C_OPTS::verilog_ostr() << "        endcase" << std::endl;
        C_OPTS::verilog_ostr() << "      end" << std::endl;
      }
      if(InstrInfo<MOV_S_R>::counter())
      {
        C_OPTS::verilog_ostr() << "      " << to_string(image.OP_BITS) << "'d";
        C_OPTS::verilog_ostr() << to_string(InstrInfo<MOV_S_R>::opcode()) << ": //mov_s_r" << std::endl;
        C_OPTS::verilog_ostr() << "      begin" << std::endl;
        C_OPTS::verilog_ostr() << "        case(stage)" << std::endl;
        C_OPTS::verilog_ostr() << "          6'd0:" << std::endl;
        C_OPTS::verilog_ostr() << "          begin" << std::endl;
        C_OPTS::verilog_ostr() << "            abus_o <= abus_o + 1'b1;" << std::endl;
        C_OPTS::verilog_ostr() << "            stage  <= stage + 1'b1;" << std::endl;
        C_OPTS::verilog_ostr() << "          end" << std::endl;
        C_OPTS::verilog_ostr() << "          6'd1:" << std::endl;
        C_OPTS::verilog_ostr() << "          begin" << std::endl;
        C_OPTS::verilog_ostr() << "            abus_o <= dsp + dbus_i;" << std::endl;
        C_OPTS::verilog_ostr() << "            dbus_o <= regs[command[RHI:RLO]];" << std::endl;
        C_OPTS::verilog_ostr() << "            wren   <= 1'b1;" << std::endl;
        C_OPTS::verilog_ostr() << "            stage  <= stage + 1'b1;" << std::endl;
        C_OPTS::verilog_ostr() << "          end" << std::endl;
        C_OPTS::verilog_ostr() << "          6'd2:" << std::endl;
        C_OPTS::verilog_ostr() << "          begin" << std::endl;
        C_OPTS::verilog_ostr() << "            wren   <= 1'b0;" << std::endl;
        C_OPTS::verilog_ostr() << "            cp     <= cp + 2'd2;" << std::endl;
        C_OPTS::verilog_ostr() << "            abus_o <= cp + 2'd2;" << std::endl;
        C_OPTS::verilog_ostr() << "            stage  <= 'b0;" << std::endl;
        C_OPTS::verilog_ostr() << "          end" << std::endl;
        C_OPTS::verilog_ostr() << "        endcase" << std::endl;
        C_OPTS::verilog_ostr() << "      end" << std::endl;
      }
      if(InstrInfo<SP_TO_R>::counter())
      {
        C_OPTS::verilog_ostr() << "      " << to_string(image.OP_BITS) << "'d";
        C_OPTS::verilog_ostr() << to_string(InstrInfo<SP_TO_R>::opcode()) << ": //sp_to_r" << std::endl;
        C_OPTS::verilog_ostr() << "      begin" << std::endl;
        C_OPTS::verilog_ostr() << "        case(stage)" << std::endl;
        C_OPTS::verilog_ostr() << "          6'd0:" << std::endl;
        C_OPTS::verilog_ostr() << "          begin" << std::endl;
        C_OPTS::verilog_ostr() << "            abus_o <= abus_o + 1'b1;" << std::endl;
        C_OPTS::verilog_ostr() << "            stage  <= stage + 1'b1;" << std::endl;
        C_OPTS::verilog_ostr() << "          end" << std::endl;
        C_OPTS::verilog_ostr() << "          6'd1:" << std::endl;
        C_OPTS::verilog_ostr() << "          begin" << std::endl;
        C_OPTS::verilog_ostr() << "            regs[command[LHI:LLO]] <= dsp + dbus_i;" << std::endl;
        C_OPTS::verilog_ostr() << "            cp     <= cp + 2'd2;" << std::endl;
        C_OPTS::verilog_ostr() << "            abus_o <= cp + 2'd2;" << std::endl;
        C_OPTS::verilog_ostr() << "            stage  <= 'b0;" << std::endl;
        C_OPTS::verilog_ostr() << "          end" << std::endl;
        C_OPTS::verilog_ostr() << "        endcase" << std::endl;
        C_OPTS::verilog_ostr() << "      end" << std::endl;
      }
      if(InstrInfo<JMP_LABEL>::counter())
      {
        C_OPTS::verilog_ostr() << "      " << to_string(image.OP_BITS) << "'d";
        C_OPTS::verilog_ostr() << to_string(InstrInfo<JMP_LABEL>::opcode()) << ": //jmp" << std::endl;
        C_OPTS::verilog_ostr() << "      begin" << std::endl;
        C_OPTS::verilog_ostr() << "        case(stage)" << std::endl;
        C_OPTS::verilog_ostr() << "          6'd0:" << std::endl;
        C_OPTS::verilog_ostr() << "          begin" << std::endl;
        C_OPTS::verilog_ostr() << "            abus_o <= abus_o + 1'b1;" << std::endl;
        C_OPTS::verilog_ostr() << "            stage  <= stage + 1'b1;" << std::endl;
        C_OPTS::verilog_ostr() << "          end" << std::endl;
        C_OPTS::verilog_ostr() << "          6'd1:" << std::endl;
        C_OPTS::verilog_ostr() << "          begin" << std::endl;
        C_OPTS::verilog_ostr() << "            cp     <= dbus_i;" << std::endl;
        C_OPTS::verilog_ostr() << "            abus_o <= dbus_i;" << std::endl;
        C_OPTS::verilog_ostr() << "            stage  <= 'b0;" << std::endl;
        C_OPTS::verilog_ostr() << "          end" << std::endl;
        C_OPTS::verilog_ostr() << "        endcase" << std::endl;
        C_OPTS::verilog_ostr() << "      end" << std::endl;
      }
      if(InstrInfo<JNE_LABEL>::counter())
      {
        C_OPTS::verilog_ostr() << "      " << to_string(image.OP_BITS) << "'d";
        C_OPTS::verilog_ostr() << to_string(InstrInfo<JNE_LABEL>::opcode()) << ": //jne" << std::endl;
        C_OPTS::verilog_ostr() << "      begin" << std::endl;
        C_OPTS::verilog_ostr() << "        if(~z_f)" << std::endl;
        C_OPTS::verilog_ostr() << "        begin" << std::endl;
        C_OPTS::verilog_ostr() << "          case(stage)" << std::endl;
        C_OPTS::verilog_ostr() << "            6'd0:" << std::endl;
        C_OPTS::verilog_ostr() << "            begin" << std::endl;
        C_OPTS::verilog_ostr() << "              abus_o <= abus_o + 1'b1;" << std::endl;
        C_OPTS::verilog_ostr() << "              stage  <= stage + 1'b1;" << std::endl;
        C_OPTS::verilog_ostr() << "            end" << std::endl;
        C_OPTS::verilog_ostr() << "            6'd1:" << std::endl;
        C_OPTS::verilog_ostr() << "            begin" << std::endl;
        C_OPTS::verilog_ostr() << "              cp     <= dbus_i;" << std::endl;
        C_OPTS::verilog_ostr() << "              abus_o <= dbus_i;" << std::endl;
        C_OPTS::verilog_ostr() << "              stage  <= 'b0;" << std::endl;
        C_OPTS::verilog_ostr() << "            end" << std::endl;
        C_OPTS::verilog_ostr() << "          endcase" << std::endl;
        C_OPTS::verilog_ostr() << "        end" << std::endl;
        C_OPTS::verilog_ostr() << "        else" << std::endl;
        C_OPTS::verilog_ostr() << "        begin" << std::endl;
        C_OPTS::verilog_ostr() << "          cp     <= cp + 2'd2;" << std::endl;
        C_OPTS::verilog_ostr() << "          abus_o <= cp + 2'd2;" << std::endl;
        C_OPTS::verilog_ostr() << "        end" << std::endl;
        C_OPTS::verilog_ostr() << "      end" << std::endl;
      }
      if(InstrInfo<JEQ_LABEL>::counter())
      {
        C_OPTS::verilog_ostr() << "      " << to_string(image.OP_BITS) << "'d";
        C_OPTS::verilog_ostr() << to_string(InstrInfo<JEQ_LABEL>::opcode()) << ": //jeq" << std::endl;
        C_OPTS::verilog_ostr() << "      begin" << std::endl;
        C_OPTS::verilog_ostr() << "        if(z_f)" << std::endl;
        C_OPTS::verilog_ostr() << "        begin" << std::endl;
        C_OPTS::verilog_ostr() << "          case(stage)" << std::endl;
        C_OPTS::verilog_ostr() << "            6'd0:" << std::endl;
        C_OPTS::verilog_ostr() << "            begin" << std::endl;
        C_OPTS::verilog_ostr() << "              abus_o <= abus_o + 1'b1;" << std::endl;
        C_OPTS::verilog_ostr() << "              stage  <= stage + 1'b1;" << std::endl;
        C_OPTS::verilog_ostr() << "            end" << std::endl;
        C_OPTS::verilog_ostr() << "            6'd1:" << std::endl;
        C_OPTS::verilog_ostr() << "            begin" << std::endl;
        C_OPTS::verilog_ostr() << "              cp     <= dbus_i;" << std::endl;
        C_OPTS::verilog_ostr() << "              abus_o <= dbus_i;" << std::endl;
        C_OPTS::verilog_ostr() << "              stage  <= 'b0;" << std::endl;
        C_OPTS::verilog_ostr() << "            end" << std::endl;
        C_OPTS::verilog_ostr() << "          endcase" << std::endl;
        C_OPTS::verilog_ostr() << "        end" << std::endl;
        C_OPTS::verilog_ostr() << "        else" << std::endl;
        C_OPTS::verilog_ostr() << "        begin" << std::endl;
        C_OPTS::verilog_ostr() << "          cp     <= cp + 2'd2;" << std::endl;
        C_OPTS::verilog_ostr() << "          abus_o <= cp + 2'd2;" << std::endl;
        C_OPTS::verilog_ostr() << "        end" << std::endl;
        C_OPTS::verilog_ostr() << "      end" << std::endl;
      }
      if(InstrInfo<JLE_LABEL>::counter())
      {
        C_OPTS::verilog_ostr() << "      " << to_string(image.OP_BITS) << "'d";
        C_OPTS::verilog_ostr() << to_string(InstrInfo<JLE_LABEL>::opcode()) << ": //jle" << std::endl;
        C_OPTS::verilog_ostr() << "      begin" << std::endl;
        C_OPTS::verilog_ostr() << "        if(l_f)" << std::endl;
        C_OPTS::verilog_ostr() << "        begin" << std::endl;
        C_OPTS::verilog_ostr() << "          case(stage)" << std::endl;
        C_OPTS::verilog_ostr() << "            6'd0:" << std::endl;
        C_OPTS::verilog_ostr() << "            begin" << std::endl;
        C_OPTS::verilog_ostr() << "              abus_o <= abus_o + 1'b1;" << std::endl;
        C_OPTS::verilog_ostr() << "              stage  <= stage + 1'b1;" << std::endl;
        C_OPTS::verilog_ostr() << "            end" << std::endl;
        C_OPTS::verilog_ostr() << "            6'd1:" << std::endl;
        C_OPTS::verilog_ostr() << "            begin" << std::endl;
        C_OPTS::verilog_ostr() << "              cp     <= dbus_i;" << std::endl;
        C_OPTS::verilog_ostr() << "              abus_o <= dbus_i;" << std::endl;
        C_OPTS::verilog_ostr() << "              stage  <= 'b0;" << std::endl;
        C_OPTS::verilog_ostr() << "            end" << std::endl;
        C_OPTS::verilog_ostr() << "          endcase" << std::endl;
        C_OPTS::verilog_ostr() << "        end" << std::endl;
        C_OPTS::verilog_ostr() << "        else" << std::endl;
        C_OPTS::verilog_ostr() << "        begin" << std::endl;
        C_OPTS::verilog_ostr() << "          cp     <= cp + 2'd2;" << std::endl;
        C_OPTS::verilog_ostr() << "          abus_o <= cp + 2'd2;" << std::endl;
        C_OPTS::verilog_ostr() << "        end" << std::endl;
        C_OPTS::verilog_ostr() << "      end" << std::endl;
      }
      if(InstrInfo<JGR_LABEL>::counter())
      {
        C_OPTS::verilog_ostr() << "      " << to_string(image.OP_BITS) << "'d";
        C_OPTS::verilog_ostr() << to_string(InstrInfo<JGR_LABEL>::opcode()) << ": //jgr" << std::endl;
        C_OPTS::verilog_ostr() << "      begin" << std::endl;
        C_OPTS::verilog_ostr() << "        if(g_f)" << std::endl;
        C_OPTS::verilog_ostr() << "        begin" << std::endl;
        C_OPTS::verilog_ostr() << "          case(stage)" << std::endl;
        C_OPTS::verilog_ostr() << "            6'd0:" << std::endl;
        C_OPTS::verilog_ostr() << "            begin" << std::endl;
        C_OPTS::verilog_ostr() << "              abus_o <= abus_o + 1'b1;" << std::endl;
        C_OPTS::verilog_ostr() << "              stage  <= stage + 1'b1;" << std::endl;
        C_OPTS::verilog_ostr() << "            end" << std::endl;
        C_OPTS::verilog_ostr() << "            6'd1:" << std::endl;
        C_OPTS::verilog_ostr() << "            begin" << std::endl;
        C_OPTS::verilog_ostr() << "              cp     <= dbus_i;" << std::endl;
        C_OPTS::verilog_ostr() << "              abus_o <= dbus_i;" << std::endl;
        C_OPTS::verilog_ostr() << "              stage  <= 'b0;" << std::endl;
        C_OPTS::verilog_ostr() << "            end" << std::endl;
        C_OPTS::verilog_ostr() << "          endcase" << std::endl;
        C_OPTS::verilog_ostr() << "        end" << std::endl;
        C_OPTS::verilog_ostr() << "        else" << std::endl;
        C_OPTS::verilog_ostr() << "        begin" << std::endl;
        C_OPTS::verilog_ostr() << "          cp     <= cp + 2'd2;" << std::endl;
        C_OPTS::verilog_ostr() << "          abus_o <= cp + 2'd2;" << std::endl;
        C_OPTS::verilog_ostr() << "        end" << std::endl;
        C_OPTS::verilog_ostr() << "      end" << std::endl;
      }
      if(InstrInfo<JNL_LABEL>::counter())
      {
        C_OPTS::verilog_ostr() << "      " << to_string(image.OP_BITS) << "'d";
        C_OPTS::verilog_ostr() << to_string(InstrInfo<JNL_LABEL>::opcode()) << ": //jnl" << std::endl;
        C_OPTS::verilog_ostr() << "      begin" << std::endl;
        C_OPTS::verilog_ostr() << "        if(~l_f)" << std::endl;
        C_OPTS::verilog_ostr() << "        begin" << std::endl;
        C_OPTS::verilog_ostr() << "          case(stage)" << std::endl;
        C_OPTS::verilog_ostr() << "            6'd0:" << std::endl;
        C_OPTS::verilog_ostr() << "            begin" << std::endl;
        C_OPTS::verilog_ostr() << "              abus_o <= abus_o + 1'b1;" << std::endl;
        C_OPTS::verilog_ostr() << "              stage  <= stage + 1'b1;" << std::endl;
        C_OPTS::verilog_ostr() << "            end" << std::endl;
        C_OPTS::verilog_ostr() << "            6'd1:" << std::endl;
        C_OPTS::verilog_ostr() << "            begin" << std::endl;
        C_OPTS::verilog_ostr() << "              cp     <= dbus_i;" << std::endl;
        C_OPTS::verilog_ostr() << "              abus_o <= dbus_i;" << std::endl;
        C_OPTS::verilog_ostr() << "              stage  <= 'b0;" << std::endl;
        C_OPTS::verilog_ostr() << "            end" << std::endl;
        C_OPTS::verilog_ostr() << "          endcase" << std::endl;
        C_OPTS::verilog_ostr() << "        end" << std::endl;
        C_OPTS::verilog_ostr() << "        else" << std::endl;
        C_OPTS::verilog_ostr() << "        begin" << std::endl;
        C_OPTS::verilog_ostr() << "          cp     <= cp + 2'd2;" << std::endl;
        C_OPTS::verilog_ostr() << "          abus_o <= cp + 2'd2;" << std::endl;
        C_OPTS::verilog_ostr() << "        end" << std::endl;
        C_OPTS::verilog_ostr() << "      end" << std::endl;
      }
      if(InstrInfo<JNG_LABEL>::counter())
      {
        C_OPTS::verilog_ostr() << "      " << to_string(image.OP_BITS) << "'d";
        C_OPTS::verilog_ostr() << to_string(InstrInfo<JNG_LABEL>::opcode()) << ": //jng" << std::endl;
        C_OPTS::verilog_ostr() << "      begin" << std::endl;
        C_OPTS::verilog_ostr() << "        if(~g_f)" << std::endl;
        C_OPTS::verilog_ostr() << "        begin" << std::endl;
        C_OPTS::verilog_ostr() << "          case(stage)" << std::endl;
        C_OPTS::verilog_ostr() << "            6'd0:" << std::endl;
        C_OPTS::verilog_ostr() << "            begin" << std::endl;
        C_OPTS::verilog_ostr() << "              abus_o <= abus_o + 1'b1;" << std::endl;
        C_OPTS::verilog_ostr() << "              stage  <= stage + 1'b1;" << std::endl;
        C_OPTS::verilog_ostr() << "            end" << std::endl;
        C_OPTS::verilog_ostr() << "            6'd1:" << std::endl;
        C_OPTS::verilog_ostr() << "            begin" << std::endl;
        C_OPTS::verilog_ostr() << "              cp     <= dbus_i;" << std::endl;
        C_OPTS::verilog_ostr() << "              abus_o <= dbus_i;" << std::endl;
        C_OPTS::verilog_ostr() << "              stage  <= 'b0;" << std::endl;
        C_OPTS::verilog_ostr() << "            end" << std::endl;
        C_OPTS::verilog_ostr() << "          endcase" << std::endl;
        C_OPTS::verilog_ostr() << "        end" << std::endl;
        C_OPTS::verilog_ostr() << "        else" << std::endl;
        C_OPTS::verilog_ostr() << "        begin" << std::endl;
        C_OPTS::verilog_ostr() << "          cp     <= cp + 2'd2;" << std::endl;
        C_OPTS::verilog_ostr() << "          abus_o <= cp + 2'd2;" << std::endl;
        C_OPTS::verilog_ostr() << "        end" << std::endl;
        C_OPTS::verilog_ostr() << "      end" << std::endl;
      }
      if(InstrInfo<CALL_LABEL>::counter())
      {
        C_OPTS::verilog_ostr() << "      " << to_string(image.OP_BITS) << "'d";
        C_OPTS::verilog_ostr() << to_string(InstrInfo<CALL_LABEL>::opcode()) << ": //call" << std::endl;
        C_OPTS::verilog_ostr() << "      begin" << std::endl;
        C_OPTS::verilog_ostr() << "        case(stage)" << std::endl;
        C_OPTS::verilog_ostr() << "          6'd0:" << std::endl;
        C_OPTS::verilog_ostr() << "          begin" << std::endl;
        C_OPTS::verilog_ostr() << "            abus_o <= asp;" << std::endl;
        C_OPTS::verilog_ostr() << "            dbus_o <= cp + 2'd2;" << std::endl;
        C_OPTS::verilog_ostr() << "            wren   <= 1'b1;" << std::endl;
        C_OPTS::verilog_ostr() << "            stage  <= stage + 1'b1;" << std::endl;
        C_OPTS::verilog_ostr() << "          end" << std::endl;
        C_OPTS::verilog_ostr() << "          6'd1:" << std::endl;
        C_OPTS::verilog_ostr() << "          begin" << std::endl;
        C_OPTS::verilog_ostr() << "            wren   <= 1'b0;" << std::endl;
        C_OPTS::verilog_ostr() << "            asp    <= asp + 1'b1;" << std::endl;
        C_OPTS::verilog_ostr() << "            abus_o <= cp + 1'b1;" << std::endl;
        C_OPTS::verilog_ostr() << "            stage  <= stage + 1'b1;" << std::endl;
        C_OPTS::verilog_ostr() << "          end" << std::endl;
        C_OPTS::verilog_ostr() << "          6'd2:" << std::endl;
        C_OPTS::verilog_ostr() << "          begin" << std::endl;
        C_OPTS::verilog_ostr() << "            cp     <= dbus_i;" << std::endl;
        C_OPTS::verilog_ostr() << "            abus_o <= dbus_i;" << std::endl;
        C_OPTS::verilog_ostr() << "            stage  <= 'b0;" << std::endl;
        C_OPTS::verilog_ostr() << "          end" << std::endl;
        C_OPTS::verilog_ostr() << "        endcase" << std::endl;
        C_OPTS::verilog_ostr() << "      end" << std::endl;
      }
      if(InstrInfo<PUSH_R>::counter())
      {
        C_OPTS::verilog_ostr() << "      " << to_string(image.OP_BITS) << "'d";
        C_OPTS::verilog_ostr() << to_string(InstrInfo<PUSH_R>::opcode()) << ": // push_r" << std::endl;
        C_OPTS::verilog_ostr() << "      begin" << std::endl;
        C_OPTS::verilog_ostr() << "        case(stage)" << std::endl;
        C_OPTS::verilog_ostr() << "          6'd0:" << std::endl;
        C_OPTS::verilog_ostr() << "          begin" << std::endl;
        C_OPTS::verilog_ostr() << "            abus_o <= dsp;" << std::endl;
        C_OPTS::verilog_ostr() << "            dbus_o <= regs[command[LHI:LLO]];" << std::endl;
        C_OPTS::verilog_ostr() << "            wren   <= 1'b1;" << std::endl;
        C_OPTS::verilog_ostr() << "            stage  <= stage + 1'b1;" << std::endl;
        C_OPTS::verilog_ostr() << "          end" << std::endl;
        C_OPTS::verilog_ostr() << "          6'd1:" << std::endl;
        C_OPTS::verilog_ostr() << "          begin" << std::endl;
        C_OPTS::verilog_ostr() << "            wren   <= 1'b0;" << std::endl;
        C_OPTS::verilog_ostr() << "            dsp    <= dsp - 1'b1;" << std::endl;
        C_OPTS::verilog_ostr() << "            cp     <= cp + 1'b1;" << std::endl;
        C_OPTS::verilog_ostr() << "            abus_o <= cp + 1'b1;" << std::endl;
        C_OPTS::verilog_ostr() << "            stage  <= 'b0;" << std::endl;
        C_OPTS::verilog_ostr() << "          end" << std::endl;
        C_OPTS::verilog_ostr() << "        endcase" << std::endl;
        C_OPTS::verilog_ostr() << "      end" << std::endl;
      }
      if(InstrInfo<POP_R>::counter())
      {
        C_OPTS::verilog_ostr() << "      " << to_string(image.OP_BITS) << "'d";
        C_OPTS::verilog_ostr() << to_string(InstrInfo<POP_R>::opcode()) << ": //pop_r" << std::endl;
        C_OPTS::verilog_ostr() << "      begin" << std::endl;
        C_OPTS::verilog_ostr() << "        case(stage)" << std::endl;
        C_OPTS::verilog_ostr() << "          6'd0:" << std::endl;
        C_OPTS::verilog_ostr() << "          begin" << std::endl;
        C_OPTS::verilog_ostr() << "            abus_o <= dsp + 1'b1;" << std::endl;
        C_OPTS::verilog_ostr() << "            dsp    <= dsp + 1'b1;" << std::endl;
        C_OPTS::verilog_ostr() << "            stage  <= stage + 1'b1;" << std::endl;
        C_OPTS::verilog_ostr() << "          end" << std::endl;
        C_OPTS::verilog_ostr() << "          6'd1:" << std::endl;
        C_OPTS::verilog_ostr() << "          begin" << std::endl;
        C_OPTS::verilog_ostr() << "            regs[command[LHI:LLO]] <= dbus_i;" << std::endl;
        C_OPTS::verilog_ostr() << "            stage  <= 'b0;" << std::endl;
        C_OPTS::verilog_ostr() << "            cp     <= cp + 1'b1;" << std::endl;
        C_OPTS::verilog_ostr() << "            abus_o <= cp + 1'b1;" << std::endl;
        C_OPTS::verilog_ostr() << "          end" << std::endl;
        C_OPTS::verilog_ostr() << "        endcase" << std::endl;
        C_OPTS::verilog_ostr() << "      end" << std::endl;
      }
      if(InstrInfo<INC_R>::counter())
      {
        C_OPTS::verilog_ostr() << "      " << to_string(image.OP_BITS) << "'d";
        C_OPTS::verilog_ostr() << to_string(InstrInfo<INC_R>::opcode()) << ": //inc_r" << std::endl;
        C_OPTS::verilog_ostr() << "      begin" << std::endl;
        C_OPTS::verilog_ostr() << "        regs[command[LHI:LLO]] <= regs[command[LHI:LLO]] + 1'b1;" << std::endl;
        C_OPTS::verilog_ostr() << "        z_f    <= inc_z_f;" << std::endl;
        C_OPTS::verilog_ostr() << "        c_f    <= inc_c_f;" << std::endl;
        C_OPTS::verilog_ostr() << "        cp     <= cp + 1'b1;" << std::endl;
        C_OPTS::verilog_ostr() << "        abus_o <= cp + 1'b1;" << std::endl;
        C_OPTS::verilog_ostr() << "      end" << std::endl;
      }
      if(InstrInfo<DEC_R>::counter())
      {
        C_OPTS::verilog_ostr() << "      " << to_string(image.OP_BITS) << "'d";
        C_OPTS::verilog_ostr() << to_string(InstrInfo<DEC_R>::opcode()) << ": //dec_r" << std::endl;
        C_OPTS::verilog_ostr() << "      begin" << std::endl;
        C_OPTS::verilog_ostr() << "        regs[command[LHI:LLO]] <= regs[command[LHI:LLO]] - 1'b1;" << std::endl;
        C_OPTS::verilog_ostr() << "        z_f    <= dec_z_f;" << std::endl;
        C_OPTS::verilog_ostr() << "        c_f    <= dec_c_f;" << std::endl;
        C_OPTS::verilog_ostr() << "        cp     <= cp + 1'b1;" << std::endl;
        C_OPTS::verilog_ostr() << "        abus_o <= cp + 1'b1;" << std::endl;
        C_OPTS::verilog_ostr() << "      end" << std::endl;
      }
      if(InstrInfo<RET>::counter())
      {
        C_OPTS::verilog_ostr() << "      " << to_string(image.OP_BITS) << "'d";
        C_OPTS::verilog_ostr() << to_string(InstrInfo<RET>::opcode()) << ": //ret" << std::endl;
        C_OPTS::verilog_ostr() << "      begin" << std::endl;
        C_OPTS::verilog_ostr() << "        case(stage)" << std::endl;
        C_OPTS::verilog_ostr() << "          6'd0:" << std::endl;
        C_OPTS::verilog_ostr() << "          begin" << std::endl;
        C_OPTS::verilog_ostr() << "            abus_o <= asp - 1'b1;" << std::endl;
        C_OPTS::verilog_ostr() << "            stage  <= stage + 1'b1;" << std::endl;
        C_OPTS::verilog_ostr() << "          end" << std::endl;
        C_OPTS::verilog_ostr() << "          6'd1:" << std::endl;
        C_OPTS::verilog_ostr() << "          begin" << std::endl;
        C_OPTS::verilog_ostr() << "            asp    <= asp - 1'b1;" << std::endl;
        C_OPTS::verilog_ostr() << "            cp     <= dbus_i;" << std::endl;
        C_OPTS::verilog_ostr() << "            abus_o <= dbus_i;" << std::endl;
        C_OPTS::verilog_ostr() << "            stage  <= 'd0;" << std::endl;
        C_OPTS::verilog_ostr() << "          end" << std::endl;
        C_OPTS::verilog_ostr() << "        endcase" << std::endl;
        C_OPTS::verilog_ostr() << "      end" << std::endl;
      }
      if(InstrInfo<LS_R>::counter())
      {
        C_OPTS::verilog_ostr() << "      " << to_string(image.OP_BITS) << "'d";
        C_OPTS::verilog_ostr() << to_string(InstrInfo<LS_R>::opcode()) << ": //ls_r" << std::endl;
        C_OPTS::verilog_ostr() << "      begin" << std::endl;
        C_OPTS::verilog_ostr() << "        regs[command[LHI:LLO]] <= ls_result;" << std::endl;
        C_OPTS::verilog_ostr() << "        c_f    <= ls_c_f;" << std::endl;
        C_OPTS::verilog_ostr() << "        z_f    <= ls_z_f;" << std::endl;
        C_OPTS::verilog_ostr() << "        cp     <= cp + 1'b1;" << std::endl;
        C_OPTS::verilog_ostr() << "        abus_o <= cp + 1'b1;" << std::endl;
        C_OPTS::verilog_ostr() << "      end" << std::endl;
      }
      if(InstrInfo<RS_R>::counter())
      {
        C_OPTS::verilog_ostr() << "      " << to_string(image.OP_BITS) << "'d";
        C_OPTS::verilog_ostr() << to_string(InstrInfo<RS_R>::opcode()) << ": //rs_r" << std::endl;
        C_OPTS::verilog_ostr() << "      begin" << std::endl;
        C_OPTS::verilog_ostr() << "        regs[command[LHI:LLO]] <= rs_result;" << std::endl;
        C_OPTS::verilog_ostr() << "        c_f    <= rs_c_f;" << std::endl;
        C_OPTS::verilog_ostr() << "        z_f    <= rs_z_f;" << std::endl;
        C_OPTS::verilog_ostr() << "        cp     <= cp + 1'b1;" << std::endl;
        C_OPTS::verilog_ostr() << "        abus_o <= cp + 1'b1;" << std::endl;
        C_OPTS::verilog_ostr() << "      end" << std::endl;
      }
      if(InstrInfo<LS_R_R>::counter())
      {
        C_OPTS::verilog_ostr() << "      " << to_string(image.OP_BITS) << "'d";
        C_OPTS::verilog_ostr() << to_string(InstrInfo<LS_R_R>::opcode()) << ": //ls_r_r" << std::endl;
        C_OPTS::verilog_ostr() << "      begin" << std::endl;
        C_OPTS::verilog_ostr() << "        case(stage)" << std::endl;
        C_OPTS::verilog_ostr() << "          6'd0:" << std::endl;
        C_OPTS::verilog_ostr() << "          begin" << std::endl;
        C_OPTS::verilog_ostr() << "            shift_counter <= regs[command[RHI:RLO]];" << std::endl;
        C_OPTS::verilog_ostr() << "            stage         <= stage + 1'b1;" << std::endl;
        C_OPTS::verilog_ostr() << "            c_f           <= 1'b0;" << std::endl;
        C_OPTS::verilog_ostr() << "          end" << std::endl;
        C_OPTS::verilog_ostr() << "          6'd1:" << std::endl;
        C_OPTS::verilog_ostr() << "          begin" << std::endl;
        C_OPTS::verilog_ostr() << "            if(|shift_counter)" << std::endl;
        C_OPTS::verilog_ostr() << "            begin" << std::endl;
        C_OPTS::verilog_ostr() << "              regs[command[LHI:LLO]] <= ls_result;" << std::endl;
        C_OPTS::verilog_ostr() << "              c_f                    <= ls_c_f | c_f;" << std::endl;
        C_OPTS::verilog_ostr() << "              shift_counter          <= shift_counter - 1'b1;" << std::endl;
        C_OPTS::verilog_ostr() << "            end" << std::endl;
        C_OPTS::verilog_ostr() << "            else" << std::endl;
        C_OPTS::verilog_ostr() << "            begin" << std::endl;
        C_OPTS::verilog_ostr() << "              stage  <= 'b0;" << std::endl;
        C_OPTS::verilog_ostr() << "              cp     <= cp + 1'b1;" << std::endl;
        C_OPTS::verilog_ostr() << "              abus_o <= cp + 1'b1;" << std::endl;
        C_OPTS::verilog_ostr() << "            end" << std::endl;
        C_OPTS::verilog_ostr() << "          end" << std::endl;
        C_OPTS::verilog_ostr() << "        endcase" << std::endl;
        C_OPTS::verilog_ostr() << "      end" << std::endl;
      }
      if(InstrInfo<RS_R_R>::counter())
      {
        C_OPTS::verilog_ostr() << "      " << to_string(image.OP_BITS) << "'d";
        C_OPTS::verilog_ostr() << to_string(InstrInfo<RS_R_R>::opcode()) << ": //rs_r_r" << std::endl;
        C_OPTS::verilog_ostr() << "      begin" << std::endl;
        C_OPTS::verilog_ostr() << "        case(stage)" << std::endl;
        C_OPTS::verilog_ostr() << "          6'd0:" << std::endl;
        C_OPTS::verilog_ostr() << "          begin" << std::endl;
        C_OPTS::verilog_ostr() << "            shift_counter <= regs[command[RHI:RLO]];" << std::endl;
        C_OPTS::verilog_ostr() << "            stage         <= stage + 1'b1;" << std::endl;
        C_OPTS::verilog_ostr() << "            c_f           <= 1'b0;" << std::endl;
        C_OPTS::verilog_ostr() << "          end" << std::endl;
        C_OPTS::verilog_ostr() << "          6'd1:" << std::endl;
        C_OPTS::verilog_ostr() << "          begin" << std::endl;
        C_OPTS::verilog_ostr() << "            if(|shift_counter)" << std::endl;
        C_OPTS::verilog_ostr() << "            begin" << std::endl;
        C_OPTS::verilog_ostr() << "              regs[command[LHI:LLO]] <= rs_result;" << std::endl;
        C_OPTS::verilog_ostr() << "              c_f                    <= rs_c_f | c_f;" << std::endl;
        C_OPTS::verilog_ostr() << "              shift_counter          <= shift_counter - 1'b1;" << std::endl;
        C_OPTS::verilog_ostr() << "            end" << std::endl;
        C_OPTS::verilog_ostr() << "            else" << std::endl;
        C_OPTS::verilog_ostr() << "            begin" << std::endl;
        C_OPTS::verilog_ostr() << "              stage  <= 'b0;" << std::endl;
        C_OPTS::verilog_ostr() << "              cp     <= cp + 1'b1;" << std::endl;
        C_OPTS::verilog_ostr() << "              abus_o <= cp + 1'b1;" << std::endl;
        C_OPTS::verilog_ostr() << "            end" << std::endl;
        C_OPTS::verilog_ostr() << "          end" << std::endl;
        C_OPTS::verilog_ostr() << "        endcase" << std::endl;
        C_OPTS::verilog_ostr() << "      end" << std::endl;        
      }
      C_OPTS::verilog_ostr() << "    endcase" << std::endl;
      C_OPTS::verilog_ostr() << "  end" << std::endl;
      C_OPTS::verilog_ostr() << "endmodule" << std::endl;
    }
    
    void compile_code()
    {
      C_OPTS::code_ostr() << "--Genereted by SomeAsm" << std::endl;
      C_OPTS::code_ostr() << "--opcode bits = " << image.OP_BITS << std::endl;
      C_OPTS::code_ostr() << "--llo = " << image.LLO << std::endl;
      C_OPTS::code_ostr() << "--lhi = " << image.LHI << std::endl;
      C_OPTS::code_ostr() << "--rlo = " << image.RLO << std::endl;
      C_OPTS::code_ostr() << "--rhi = " << image.RHI << std::endl;
      C_OPTS::code_ostr() << std::endl;
      C_OPTS::code_ostr() << "WIDTH=" << std::to_string(C_OPTS::BITS) << ";" << std::endl;
      C_OPTS::code_ostr() << "DEPTH=" << std::to_string(image.size) << ";" << std::endl;
      C_OPTS::code_ostr() << std::endl;
      C_OPTS::code_ostr() << "ADDRESS_RADIX=UNS;" << std::endl;
      C_OPTS::code_ostr() << "DATA_RADIX=UNS;" << std::endl;
      C_OPTS::code_ostr() << std::endl;
      C_OPTS::code_ostr() << "CONTENT BEGIN" << std::endl;
      
      int addr = 0;
      for(unsigned int i = 0; i < instructions.size(); i++)
        instructions[i]->code(addr);

      C_OPTS::code_ostr() << "END;" << std::endl;
    }
  };
  
  static Image image;
  
  struct Comment : Instuction
  {
    std::string text;
    template <class T>
    Comment(const T& _text) : text(_text) {}
    int size() { return 0; }
    int bytes(int) { return 0; }
    void asm_text()
    {
      C_OPTS::asm_ostr() << ";" << text << std::endl;
    }
    void code(int&) {}
  };
  
  template <class Dummy>
  struct InstrInfo<Comment, Dummy>
  {
    static int& counter()
    {
      static int always_zero = 0;
      always_zero = 0;
      return always_zero;
    }
    
    static int& opcode()
    {
      //mb throw some?
      static int useless = -1;
      useless = -1;
      return useless;
    }
  };
  
  struct MOV_R_R : Instuction
  {
    
    int l, r;
    MOV_R_R(int _l, int _r) : l(_l), r(_r) {}
    int size() { return 1; }
    void asm_text()
    {
      C_OPTS::asm_ostr() << "  mov  " << C_OPTS::regname(l) << ", ";
      C_OPTS::asm_ostr() << C_OPTS::regname(r) << std::endl;
    }
    void code(int& addr)
    {
      C_OPTS::code_ostr() << "\t" << std::to_string(addr++) << "    :   ";
      C_OPTS::code_ostr() << to_string(InstrInfo<MOV_R_R>::opcode() | l << image.LLO | r << image.RLO);
      C_OPTS::code_ostr() << ";\t--mov_r_r\t" << to_string(InstrInfo<MOV_R_R>::opcode());
      C_OPTS::code_ostr() << " " << to_string(l) << " " << to_string(r) << std::endl;
    }
    int bytes(int)
    { return InstrInfo<MOV_R_R>::opcode() | l << image.LLO | r << image.RLO; }
  };
  
  struct MOV_R_B : Instuction
  {
    int reg, value;
    MOV_R_B(int _reg, int _value) : reg(_reg), value(_value) {}
    int size() { return 2; }
    void asm_text()
    {
      C_OPTS::asm_ostr() << "  mov  " << C_OPTS::regname(reg) << ", ";
      C_OPTS::asm_ostr() << std::to_string(value) << std::endl;
    }
    void code(int& addr)
    {
      C_OPTS::code_ostr() << "\t" << to_string(addr++) << "    :   ";
      C_OPTS::code_ostr() << to_string(InstrInfo<MOV_R_B>::opcode() | reg << image.LLO);
      C_OPTS::code_ostr() << ";--mov_r_b\t";
      C_OPTS::code_ostr() << to_string(InstrInfo<MOV_R_B>::opcode()) << " "; 
      C_OPTS::code_ostr() << to_string(reg) << std::endl;
      C_OPTS::code_ostr() << "\t" << to_string(addr++) << "    :   ";
      C_OPTS::code_ostr() << to_string(value);
      C_OPTS::code_ostr() << ";" << std::endl;
    }
    int bytes(int i)
    {
      if(i == 0)
        return InstrInfo<MOV_R_B>::opcode() | reg << image.LLO;
      else
        return value;
    }
  };
  
  struct MOV_R_STATIC_VAR_ADDRESS : Instuction
  {
    int reg, rel_address;
    MOV_R_STATIC_VAR_ADDRESS(int _reg, int _rel_address) : reg(_reg), rel_address(_rel_address) {}
    int size() { return 2; }
    void asm_text()
    {
      MOV_R_B _mov_r_b(reg, image.get_static_var_address(rel_address));
      _mov_r_b.asm_text();
    }
    void code(int& addr)
    {
      MOV_R_B _mov_r_b(reg, image.get_static_var_address(rel_address));
      _mov_r_b.code(addr);
    }
    int bytes(int i)
    {
      MOV_R_B _mov_r_b(reg, image.get_static_var_address(rel_address));
      return _mov_r_b.bytes(i);
    }
  };
  
  template <class Dummy>
  struct InstrInfo<MOV_R_STATIC_VAR_ADDRESS, Dummy>
  {
    static int& counter()
    {
      return InstrInfo<MOV_R_B, Dummy>::counter();
    }
    
    static int& opcode()
    {
      return InstrInfo<MOV_R_B, Dummy>::opcode();
    }
  };
  
  struct MOV_R_A : Instuction
  {
    int l, r;
    MOV_R_A(int _l, int _r) : l(_l), r(_r) {}
    int size() { return 1; }
    void asm_text()
    {
      C_OPTS::asm_ostr() << "  mov  " << C_OPTS::regname(l) << ", [";
      C_OPTS::asm_ostr() << C_OPTS::regname(r) << "]" << std::endl;
    }
    void code(int& addr)
    {
      C_OPTS::code_ostr() << "\t" << to_string(addr++) << "    :   ";
      C_OPTS::code_ostr() << to_string(InstrInfo<MOV_R_A>::opcode() | l << image.LLO | r << image.RLO);
      C_OPTS::code_ostr() << ";--mov_r_a\t";
      C_OPTS::code_ostr() << to_string(InstrInfo<MOV_R_A>::opcode()) << " ";
      C_OPTS::code_ostr() << to_string(l) << " " << to_string(r) << std::endl;
    }
    int bytes(int)
    { return InstrInfo<MOV_R_A>::opcode() | l << image.LLO | r << image.RLO; }
  };
  
  struct MOV_A_R : Instuction
  {
    int l, r;
    MOV_A_R(int _l, int _r) : l(_l), r(_r) {}
    int size() { return 1; }
    void asm_text()
    {
      C_OPTS::asm_ostr() << "  mov  [" << C_OPTS::regname(l) << "], ";
      C_OPTS::asm_ostr() << C_OPTS::regname(r) << std::endl;
    }
    void code(int& addr)
    {
      C_OPTS::code_ostr() << "\t" << to_string(addr++) << "    :   ";
      C_OPTS::code_ostr() << to_string(InstrInfo<MOV_A_R>::opcode() | l << image.LLO | r << image.RLO);
      C_OPTS::code_ostr() << ";--mov_a_r\t";
      C_OPTS::code_ostr() << to_string(InstrInfo<MOV_A_R>::opcode()) << " ";
      C_OPTS::code_ostr() << to_string(l) << " " << to_string(r) << std::endl;
    }
    int bytes(int)
    { return InstrInfo<MOV_A_R>::opcode() | l << image.LLO | r << image.RLO; }
  };
  
  struct PUSH_R : Instuction
  {
    int reg;
    PUSH_R(int _reg) : reg(_reg) {}
    int size() { return 1; }
    void asm_text()
    {
      C_OPTS::asm_ostr() << "  push " << C_OPTS::regname(reg) << std::endl;
    }
    void code(int& addr)
    {
      C_OPTS::code_ostr() << "\t" << to_string(addr++) << "    :   ";
      C_OPTS::code_ostr() << to_string(InstrInfo<PUSH_R>::opcode() | reg << image.LLO);
      C_OPTS::code_ostr() << ";--push_r\t";
      C_OPTS::code_ostr() << to_string(InstrInfo<PUSH_R>::opcode()) << " ";
      C_OPTS::code_ostr() << to_string(reg) << std::endl;
    }
    int bytes(int)
    { return InstrInfo<PUSH_R>::opcode() | reg << image.LLO; }
  };
  
  struct POP_R : Instuction
  {
    int reg;
    POP_R(int _reg) : reg(_reg) {}
    int size() { return 1; }
    void asm_text()
    {
      C_OPTS::asm_ostr() << "  pop  " << C_OPTS::regname(reg) << std::endl;
    }
    void code(int& addr)
    {
      C_OPTS::code_ostr() << "\t" << to_string(addr++) << "    :   ";
      C_OPTS::code_ostr() << to_string(InstrInfo<POP_R>::opcode() | reg << image.LLO);
      C_OPTS::code_ostr() << ";--pop_r\t";
      C_OPTS::code_ostr() << to_string(InstrInfo<POP_R>::opcode()) << " ";
      C_OPTS::code_ostr() << to_string(reg) << std::endl;
    }
    int bytes(int)
    { return InstrInfo<POP_R>::opcode() | reg << image.LLO; }
  };
  
  struct MOV_R_S : Instuction
  {
    int reg, offset;
    MOV_R_S(int _reg, int _offset) : reg(_reg), offset(_offset) {}
    int size() { return 2; }
    void asm_text()
    {
      C_OPTS::asm_ostr() << "  mov  " << C_OPTS::regname(reg) << ", [sp + ";
      C_OPTS::asm_ostr() << std::to_string(offset) << "]" << std::endl;
    }
    void code(int& addr)
    {
      C_OPTS::code_ostr() << "\t" << to_string(addr++) << "    :   ";
      C_OPTS::code_ostr() << to_string(InstrInfo<MOV_R_S>::opcode() | reg << image.LLO);
      C_OPTS::code_ostr() << ";--mov_r_s\t";
      C_OPTS::code_ostr() << to_string(InstrInfo<MOV_R_S>::opcode()) << " ";
      C_OPTS::code_ostr() << to_string(reg) << std::endl;
      C_OPTS::code_ostr() << "\t" << to_string(addr++) << "    :   ";
      C_OPTS::code_ostr() << to_string(offset);
      C_OPTS::code_ostr() << ";" << std::endl;
    }
    int bytes(int i)
    {
      if(i == 0)
        return InstrInfo<MOV_R_S>::opcode() | reg << image.LLO;
      else
        return offset;
    }
  };
  
  struct MOV_S_R : Instuction
  {
    int offset, reg;
    MOV_S_R(int _offset, int _reg) : offset(_offset), reg(_reg) {}
    int size() { return 2; }
    void asm_text()
    {
      C_OPTS::asm_ostr() << "  mov  [sp + " << std::to_string(offset);
      C_OPTS::asm_ostr() << "], " << C_OPTS::regname(reg) << std::endl;
    }
    void code(int& addr)
    {
      C_OPTS::code_ostr() << "\t" << to_string(addr++) << "    :   ";
      C_OPTS::code_ostr() << to_string(InstrInfo<MOV_S_R>::opcode() | reg << image.LLO);
      C_OPTS::code_ostr() << ";--mov_s_r\t";
      C_OPTS::code_ostr() << to_string(InstrInfo<MOV_S_R>::opcode()) << " ";
      C_OPTS::code_ostr() << to_string(reg) << std::endl;
      C_OPTS::code_ostr() << "\t" << to_string(addr++) << "    :   ";
      C_OPTS::code_ostr() << to_string(offset);
      C_OPTS::code_ostr() << ";" << std::endl;
    }
    int bytes(int i)
    {
      if(i == 0)
        return InstrInfo<MOV_S_R>::opcode() | reg << image.LLO;
      else
        return offset;
    }
  };
  
  struct SP_TO_R : Instuction
  {
    int reg, offset;
    SP_TO_R(int _reg, int _offset) : reg(_reg), offset(_offset) {}
    int size() { return 2; }
    void asm_text()
    {
      C_OPTS::asm_ostr() << "  mov  " << C_OPTS::regname(reg) << ", sp + ";
      C_OPTS::asm_ostr() << std::to_string(offset) << std::endl;
    }
    void code(int& addr)
    {
      C_OPTS::code_ostr() << "\t" << to_string(addr++) << "    :   ";
      C_OPTS::code_ostr() << to_string(InstrInfo<SP_TO_R>::opcode() | reg << image.LLO);
      C_OPTS::code_ostr() << ";--sp_to_r\t";
      C_OPTS::code_ostr() << to_string(InstrInfo<SP_TO_R>::opcode()) << " ";
      C_OPTS::code_ostr() << to_string(reg) << " " << to_string(offset) << std::endl;
      C_OPTS::code_ostr() << "\t" << to_string(addr++) << "    :   ";
      C_OPTS::code_ostr() << to_string(offset);
      C_OPTS::code_ostr() << ";" << std::endl;
    }
    int bytes(int i)
    {
      if(i == 0)
        return InstrInfo<SP_TO_R>::opcode() | reg << image.LLO;
      else
        return offset;
    }
  };
  
  struct LABEL : Instuction
  {
    int label_num;
    LABEL(int _label_num) : label_num(_label_num) {}
    int size() { return 0; }
    void asm_text()
    {
      C_OPTS::asm_ostr() << "label_" << std::to_string(label_num);
      C_OPTS::asm_ostr() << ":" << std::endl;
    }
    void code(int&) {}
    int bytes(int) { return 0; }
  };
  
  struct JMP_LABEL : Instuction
  {
    int label_num;
    JMP_LABEL(int _label_num) : label_num(_label_num) {}
    int size() { return 2; }
    void asm_text()
    {
      C_OPTS::asm_ostr() << "  jmp label_" << std::to_string(label_num);
      C_OPTS::asm_ostr() << std::endl;
    }
    void code(int& addr)
    {
      C_OPTS::code_ostr() << "\t" << to_string(addr++) << "    :   ";
      C_OPTS::code_ostr() << to_string(InstrInfo<JMP_LABEL>::opcode());
      C_OPTS::code_ostr() << ";--jmp" << std::endl;
      C_OPTS::code_ostr() << "\t" << to_string(addr++) << "    :   ";
      C_OPTS::code_ostr() << to_string(image.label_address[label_num]);
      C_OPTS::code_ostr() << ";" << std::endl;
    }
    int bytes(int i)
    {
      if(i == 0)
        return InstrInfo<JMP_LABEL>::opcode();
      else
        return image.label_address[label_num];
    }
  };
  
  struct JNE_LABEL : Instuction
  {
    int label_num;
    JNE_LABEL(int _label_num) : label_num(_label_num) {}
    int size() { return 2; }
    void asm_text()
    {
      C_OPTS::asm_ostr() << "  jne label_" << std::to_string(label_num);
      C_OPTS::asm_ostr() << std::endl;
    }
    void code(int& addr)
    {
      C_OPTS::code_ostr() << "\t" << to_string(addr++) << "    :   ";
      C_OPTS::code_ostr() << to_string(InstrInfo<JNE_LABEL>::opcode());
      C_OPTS::code_ostr() << ";--jne" << std::endl;
      C_OPTS::code_ostr() << "\t" << to_string(addr++) << "    :   ";
      C_OPTS::code_ostr() << to_string(image.label_address[label_num]);
      C_OPTS::code_ostr() << ";" << std::endl;
    }
    int bytes(int i)
    {
      if(i == 0)
        return InstrInfo<JNE_LABEL>::opcode();
      else
        return image.label_address[label_num];
    }
  };
  
  struct JEQ_LABEL : Instuction
  {
    int label_num;
    JEQ_LABEL(int _label_num) : label_num(_label_num) {}
    int size() { return 2; }
    void asm_text()
    {
      C_OPTS::asm_ostr() << "  jeq label_" << std::to_string(label_num);
      C_OPTS::asm_ostr() << std::endl;
    }
    void code(int& addr)
    {
      C_OPTS::code_ostr() << "\t" << to_string(addr++) << "    :   ";
      C_OPTS::code_ostr() << to_string(InstrInfo<JEQ_LABEL>::opcode());
      C_OPTS::code_ostr() << ";--jeq" << std::endl;
      C_OPTS::code_ostr() << "\t" << to_string(addr++) << "    :   ";
      C_OPTS::code_ostr() << to_string(image.label_address[label_num]);
      C_OPTS::code_ostr() << ";" << std::endl;
    }
    int bytes(int i)
    {
      if(i == 0)
        return InstrInfo<JEQ_LABEL>::opcode();
      else
        return image.label_address[label_num];
    }
  };
  
  struct JLE_LABEL : Instuction
  {
    int label_num;
    JLE_LABEL(int _label_num) : label_num(_label_num) {}
    int size() { return 2; }
    void asm_text()
    {
      C_OPTS::asm_ostr() << "  jle label_" << std::to_string(label_num);
      C_OPTS::asm_ostr() << std::endl;
    }
    void code(int& addr)
    {
      C_OPTS::code_ostr() << "\t" << to_string(addr++) << "    :   ";
      C_OPTS::code_ostr() << to_string(InstrInfo<JLE_LABEL>::opcode());
      C_OPTS::code_ostr() << ";--jle" << std::endl;
      C_OPTS::code_ostr() << "\t" << to_string(addr++) << "    :   ";
      C_OPTS::code_ostr() << to_string(image.label_address[label_num]);
      C_OPTS::code_ostr() << ";" << std::endl;
    }
    int bytes(int i)
    {
      if(i == 0)
        return InstrInfo<JLE_LABEL>::opcode();
      else
        return image.label_address[label_num];
    }
  };
  
  struct JGR_LABEL : Instuction
  {
    int label_num;
    JGR_LABEL(int _label_num) : label_num(_label_num) {}
    int size() { return 2; }
    void asm_text()
    {
      C_OPTS::asm_ostr() << "  jgr label_" << std::to_string(label_num);
      C_OPTS::asm_ostr() << std::endl;
    }
    void code(int& addr)
    {
      C_OPTS::code_ostr() << "\t" << to_string(addr++) << "    :   ";
      C_OPTS::code_ostr() << to_string(InstrInfo<JGR_LABEL>::opcode());
      C_OPTS::code_ostr() << ";--jgr" << std::endl;
      C_OPTS::code_ostr() << "\t" << to_string(addr++) << "    :   ";
      C_OPTS::code_ostr() << to_string(image.label_address[label_num]);
      C_OPTS::code_ostr() << ";" << std::endl;
    }
    int bytes(int i)
    {
      if(i == 0)
        return InstrInfo<JGR_LABEL>::opcode();
      else
        return image.label_address[label_num];
    }
  };
  
  struct JNL_LABEL : Instuction
  {
    int label_num;
    JNL_LABEL(int _label_num) : label_num(_label_num) {}
    int size() { return 2; }
    void asm_text()
    {
      C_OPTS::asm_ostr() << "  jnl label_" << std::to_string(label_num);
      C_OPTS::asm_ostr() << std::endl;
    }
    void code(int& addr)
    {
      C_OPTS::code_ostr() << "\t" << to_string(addr++) << "    :   ";
      C_OPTS::code_ostr() << to_string(InstrInfo<JNL_LABEL>::opcode());
      C_OPTS::code_ostr() << ";--jnl" << std::endl;
      C_OPTS::code_ostr() << "\t" << to_string(addr++) << "    :   ";
      C_OPTS::code_ostr() << to_string(image.label_address[label_num]);
      C_OPTS::code_ostr() << ";" << std::endl;
    }
    int bytes(int i)
    {
      if(i == 0)
        return InstrInfo<JNL_LABEL>::opcode();
      else
        return image.label_address[label_num];
    }
  };
  
  struct JNG_LABEL : Instuction
  {
    int label_num;
    JNG_LABEL(int _label_num) : label_num(_label_num) {}
    int size() { return 2; }
    void asm_text()
    {
      C_OPTS::asm_ostr() << "  jng label_" << std::to_string(label_num);
      C_OPTS::asm_ostr() << std::endl;
    }
    void code(int& addr)
    {
      C_OPTS::code_ostr() << "\t" << to_string(addr++) << "    :   ";
      C_OPTS::code_ostr() << to_string(InstrInfo<JNG_LABEL>::opcode());
      C_OPTS::code_ostr() << ";--jng" << std::endl;
      C_OPTS::code_ostr() << "\t" << to_string(addr++) << "    :   ";
      C_OPTS::code_ostr() << to_string(image.label_address[label_num]);
      C_OPTS::code_ostr() << ";" << std::endl;
    }
    int bytes(int i)
    {
      if(i == 0)
        return InstrInfo<JNG_LABEL>::opcode();
      else
        return image.label_address[label_num];
    }
  };
  
  struct CALL_LABEL : Instuction
  {
    int label_num;
    CALL_LABEL(int _label_num) : label_num(_label_num) {}
    int size() { return 2; }
    void asm_text()
    {
      C_OPTS::asm_ostr() << "  call label_" << std::to_string(label_num);
      C_OPTS::asm_ostr() << std::endl;
    }
    void code(int& addr)
    {
      C_OPTS::code_ostr() << "\t" << to_string(addr++) << "    :   ";
      C_OPTS::code_ostr() << to_string(InstrInfo<CALL_LABEL>::opcode());
      C_OPTS::code_ostr() << ";--call" << std::endl;
      C_OPTS::code_ostr() << "\t" << to_string(addr++) << "    :   ";
      C_OPTS::code_ostr() << to_string(image.label_address[label_num]);
      C_OPTS::code_ostr() << ";" << std::endl;
    }
    int bytes(int i)
    {
      if(i == 0)
        return InstrInfo<CALL_LABEL>::opcode();
      else
        return image.label_address[label_num];
    }
  };
  
  struct RET : Instuction
  {
    RET() {}
    int size() { return 1; }
    void asm_text()
    {
      C_OPTS::asm_ostr() << "  ret" << std::endl;
    }
    void code(int& addr)
    {
      C_OPTS::code_ostr() << "\t" << to_string(addr++) << "    :   ";
      C_OPTS::code_ostr() << to_string(InstrInfo<RET>::opcode());
      C_OPTS::code_ostr() << ";--ret" << std::endl;
    }
    int bytes(int)
    { return InstrInfo<RET>::opcode(); }
  };
  
  struct CMP_R_R : Instuction
  {
    int l, r;
    CMP_R_R(int _l, int _r) : l(_l), r(_r) {}
    int size() { return 1; }
    void asm_text()
    {
      C_OPTS::asm_ostr() << "  cmp  " << C_OPTS::regname(l) << ", ";
      C_OPTS::asm_ostr() << C_OPTS::regname(r) << std::endl;
    }
    void code(int &addr)
    {
      C_OPTS::code_ostr() << "\t" << to_string(addr++) << "    :   ";
      C_OPTS::code_ostr() << to_string(InstrInfo<CMP_R_R>::opcode() | l << image.LLO | r << image.RLO);
      C_OPTS::code_ostr() << ";--cmp_r_r\t";
      C_OPTS::code_ostr() << to_string(InstrInfo<CMP_R_R>::opcode()) << " ";
      C_OPTS::code_ostr() << to_string(l) << " " << to_string(r) << std::endl;
    }
    int bytes(int)
    { return InstrInfo<CMP_R_R>::opcode() | l << image.LLO | r << image.RLO; }
  };
  
  struct OR_R_R : Instuction
  {
    int l, r;
    OR_R_R(int _l, int _r) : l(_l), r(_r) {}
    int size() { return 1; }
    void asm_text()
    {
      C_OPTS::asm_ostr() << "  or   " << C_OPTS::regname(l) << ", ";
      C_OPTS::asm_ostr() << C_OPTS::regname(r);
      C_OPTS::asm_ostr() << ";" << std::endl;
    }
    void code(int& addr)
    {
      C_OPTS::code_ostr() << "\t" << to_string(addr++) << "    :   ";
      C_OPTS::code_ostr() << to_string(InstrInfo<OR_R_R>::opcode() | l << image.LLO | r << image.RLO);
      C_OPTS::code_ostr() << ";--or_r_r\t";
      C_OPTS::code_ostr() << to_string(InstrInfo<OR_R_R>::opcode()) << " ";
      C_OPTS::code_ostr() << to_string(l) << " " << to_string(r) << std::endl;
    }
    int bytes(int)
    { return InstrInfo<OR_R_R>::opcode() | l << image.LLO | r << image.RLO; }
  };
  
  struct AND_R_R : Instuction
  {
    int l, r;
    AND_R_R(int _l, int _r) : l(_l), r(_r) {}
    int size() { return 1; }
    void asm_text()
    {
      C_OPTS::asm_ostr() << "  and  " << C_OPTS::regname(l) << ", ";
      C_OPTS::asm_ostr() << C_OPTS::regname(r) << std::endl;
    }
    void code(int& addr)
    {
      C_OPTS::code_ostr() << "\t" << to_string(addr++) << "    :   ";
      C_OPTS::code_ostr() << to_string(InstrInfo<AND_R_R>::opcode() | l << image.LLO | r << image.RLO);
      C_OPTS::code_ostr() << ";--and_r_r\t";
      C_OPTS::code_ostr() << to_string(InstrInfo<AND_R_R>::opcode()) << " ";
      C_OPTS::code_ostr() << to_string(l) << " " << to_string(r) << std::endl;
    }
    int bytes(int)
    { return InstrInfo<AND_R_R>::opcode() | l << image.LLO | r << image.RLO; }
  };
  
  struct XOR_R_R : Instuction
  {
    int l, r;
    XOR_R_R(int _l, int _r) : l(_l), r(_r) {}
    int size() { return 1; }
    void asm_text()
    {
      C_OPTS::asm_ostr() << "  xor  " << C_OPTS::regname(l) << ", ";
      C_OPTS::asm_ostr() << C_OPTS::regname(r) << std::endl;
    }
    void code(int& addr)
    {
      C_OPTS::code_ostr() << "\t" << to_string(addr++) << "    :   ";
      C_OPTS::code_ostr() << to_string(InstrInfo<XOR_R_R>::opcode() | l << image.LLO | r << image.RLO);
      C_OPTS::code_ostr() << ";--xor_r_r\t";
      C_OPTS::code_ostr() << to_string(InstrInfo<XOR_R_R>::opcode()) << " ";
      C_OPTS::code_ostr() << to_string(l) << " " << to_string(r) << std::endl;
    }
    int bytes(int)
    { return InstrInfo<XOR_R_R>::opcode() | l << image.LLO | r << image.RLO; }
  };
  
  struct ADD_R_R : Instuction
  {
    int l, r;
    ADD_R_R(int _l, int _r) : l(_l), r(_r) {}
    int size() { return 1; }
    void asm_text()
    {
      C_OPTS::asm_ostr() << "  add  " << C_OPTS::regname(l) << ", ";
      C_OPTS::asm_ostr() << C_OPTS::regname(r) << std::endl;
    }
    void code(int& addr)
    {
      C_OPTS::code_ostr() << "\t" << to_string(addr++) << "    :   ";
      C_OPTS::code_ostr() << to_string(InstrInfo<ADD_R_R>::opcode() | l << image.LLO | r << image.RLO);
      C_OPTS::code_ostr() << ";--add\t";
      C_OPTS::code_ostr() << to_string(InstrInfo<ADD_R_R>::opcode()) << " ";
      C_OPTS::code_ostr() << to_string(l) << " " << to_string(r) << std::endl;
    }
    int bytes(int)
    { return InstrInfo<ADD_R_R>::opcode() | l << image.LLO | r << image.RLO; }
  };
  
  struct SUB_R_R : Instuction
  {
    int l, r;
    SUB_R_R(int _l, int _r) : l(_l), r(_r) {}
    int size() { return 1; }
    void asm_text()
    {
      C_OPTS::asm_ostr() << "  sub  " << C_OPTS::regname(l) << ", ";
      C_OPTS::asm_ostr() << C_OPTS::regname(r) << std::endl;
    }
    void code(int& addr)
    {
      C_OPTS::code_ostr() << "\t" << to_string(addr++) << "    :   ";
      C_OPTS::code_ostr() << to_string(InstrInfo<SUB_R_R>::opcode() | l << image.LLO | r << image.RLO);
      C_OPTS::code_ostr() << ";--sub_r_r\t";
      C_OPTS::code_ostr() << to_string(InstrInfo<SUB_R_R>::opcode()) << " ";
      C_OPTS::code_ostr() << to_string(l) << " " << to_string(r) << std::endl;
    }
    int bytes(int)
    { return InstrInfo<SUB_R_R>::opcode() | l << image.LLO | r << image.RLO; }
  };
  
  struct MUL_R_R : Instuction
  {
    int l, r;
    MUL_R_R(int _l, int _r) : l(_l), r(_r) {}
    int size() { return 1; }
    void asm_text()
    {
      C_OPTS::asm_ostr() << "  mul  " << C_OPTS::regname(l) << ", ";
      C_OPTS::asm_ostr() << C_OPTS::regname(r) << std::endl;
    }
    void code(int& addr)
    {
      C_OPTS::code_ostr() << "\t" << to_string(addr++) << "    :   ";
      C_OPTS::code_ostr() << to_string(InstrInfo<MUL_R_R>::opcode() | l << image.LLO | r << image.RLO);
      C_OPTS::code_ostr() << ";--mul_r_r\t";
      C_OPTS::code_ostr() << to_string(InstrInfo<MUL_R_R>::opcode()) << " ";
      C_OPTS::code_ostr() << to_string(l) << " " << to_string(r) << std::endl;
    }
    int bytes(int)
    { return InstrInfo<MUL_R_R>::opcode() | l << image.LLO | r << image.RLO; }
  };
  
  struct DIV_R_R : Instuction
  {
    int l, r;
    DIV_R_R(int _l, int _r) : l(_l), r(_r) {}
    int size() { return 1; }
    void asm_text()
    {
      C_OPTS::asm_ostr() << "  div  " << C_OPTS::regname(l) << ", ";
      C_OPTS::asm_ostr() << C_OPTS::regname(r) << std::endl;
    }
    void code(int& addr)
    {
      C_OPTS::code_ostr() << "\t" << to_string(addr++) << "    :   ";
      C_OPTS::code_ostr() << to_string(InstrInfo<DIV_R_R>::opcode() | l << image.LLO | r << image.RLO);
      C_OPTS::code_ostr() << ";--div_r_r\t";
      C_OPTS::code_ostr() << to_string(InstrInfo<DIV_R_R>::opcode()) << " ";
      C_OPTS::code_ostr() << to_string(l) << " " << to_string(r) << std::endl;
    }
    int bytes(int)
    { return InstrInfo<DIV_R_R>::opcode() | l << image.LLO | r << image.RLO; }
  };
  
  struct MOD_R_R : Instuction
  {
    int l, r;
    MOD_R_R(int _l, int _r) : l(_l), r(_r) {}
    int size() { return 1; }
    void asm_text()
    {
      C_OPTS::asm_ostr() << "  mod  " << C_OPTS::regname(l) << ", ";
      C_OPTS::asm_ostr() << C_OPTS::regname(r) << std::endl;
    }
    void code(int& addr)
    {
      C_OPTS::code_ostr() << "\t" << to_string(addr++) << "    :   ";
      C_OPTS::code_ostr() << to_string(InstrInfo<MOD_R_R>::opcode() | l << image.LLO | r << image.RLO);
      C_OPTS::code_ostr() << ";--mod_r_r\t";
      C_OPTS::code_ostr() << to_string(InstrInfo<MOD_R_R>::opcode()) << " ";
      C_OPTS::code_ostr() << to_string(l) << " " << to_string(r) << std::endl;
    }
    int bytes(int)
    { return InstrInfo<MOD_R_R>::opcode() | l << image.LLO | r << image.RLO; }
  };
  
  struct LS_R : Instuction
  {
    int reg;
    LS_R(int _reg) : reg(_reg) {}
    int size() { return 1; }
    void asm_text()
    {
      C_OPTS::asm_ostr() << "  ls   " << C_OPTS::regname(reg);
      C_OPTS::asm_ostr() << std::endl;
    }
    void code(int& addr)
    {
      C_OPTS::code_ostr() << "\t" << to_string(addr++) << "    :   ";
      C_OPTS::code_ostr() << to_string(InstrInfo<LS_R>::opcode() | reg << image.LLO);
      C_OPTS::code_ostr() << ";--ls_r\t";
      C_OPTS::code_ostr() << to_string(InstrInfo<LS_R>::opcode()) << " ";
      C_OPTS::code_ostr() << to_string(reg) << std::endl;
    }
    int bytes(int)
    { return InstrInfo<LS_R>::opcode() | reg << image.LLO; }
  };
  
  struct RS_R : Instuction
  {
    int reg;
    RS_R(int _reg) : reg(_reg) {}
    int size() { return 1; }
    void asm_text()
    {
      C_OPTS::asm_ostr() << "  rs   " << C_OPTS::regname(reg);
      C_OPTS::asm_ostr() << std::endl;
    }
    void code(int& addr)
    {
      C_OPTS::code_ostr() << "\t" << to_string(addr++) << "    :   ";
      C_OPTS::code_ostr() << to_string(InstrInfo<RS_R>::opcode() | reg << image.LLO);
      C_OPTS::code_ostr() << ";--rs_r\t";
      C_OPTS::code_ostr() << to_string(InstrInfo<RS_R>::opcode()) << " ";
      C_OPTS::code_ostr() << to_string(reg) << std::endl;
    }
    int bytes(int)
    { return InstrInfo<RS_R>::opcode() | reg << image.LLO; }
  };
  
  struct LS_R_R : Instuction
  {
    int l, r;
    LS_R_R(int _l, int _r) : l(_l), r(_r) {}
    int size() { return 1; }
    void asm_text()
    {
      C_OPTS::asm_ostr() << "  ls   " << C_OPTS::regname(l);
      C_OPTS::asm_ostr() << ", " << C_OPTS::regname(r);
      C_OPTS::asm_ostr() << std::endl;
    }
    void code(int& addr)
    {
      C_OPTS::code_ostr() << "\t" << to_string(addr++) << "    :   ";
      C_OPTS::code_ostr() << to_string(InstrInfo<LS_R_R>::opcode() | l << image.LLO | r << image.RLO);
      C_OPTS::code_ostr() << ";--ls_r_r\t";
      C_OPTS::code_ostr() << to_string(InstrInfo<LS_R_R>::opcode()) << " ";
      C_OPTS::code_ostr() << to_string(l) << " " << to_string(r) << std::endl;
    }
    int bytes(int)
    { return InstrInfo<LS_R_R>::opcode() | l << image.LLO | r << image.RLO; }
  };
  
  struct RS_R_R : Instuction
  {
    int l, r;
    RS_R_R(int _l, int _r) : l(_l), r(_r) {}
    int size() { return 1; }
    void asm_text()
    {
      C_OPTS::asm_ostr() << "  rs   " << C_OPTS::regname(l);
      C_OPTS::asm_ostr() << ", " << C_OPTS::regname(r);
      C_OPTS::asm_ostr() << std::endl;
    }
    void code(int& addr)
    {
      C_OPTS::code_ostr() << "\t" << to_string(addr++) << "    :   ";
      C_OPTS::code_ostr() << to_string(InstrInfo<RS_R_R>::opcode() | l << image.LLO | r >> image.RLO);
      C_OPTS::code_ostr() << ";--rs_r_r\t";
      C_OPTS::code_ostr() << to_string(InstrInfo<RS_R_R>::opcode()) << " ";
      C_OPTS::code_ostr() << to_string(l) << " " << to_string(r) << std::endl;
    }
    int bytes(int)
    { return InstrInfo<RS_R_R>::opcode() | l << image.LLO | r << image.RLO; }
  };
  
  struct IN_R_R : Instuction
  {
    int l, r;
    IN_R_R(int _l, int _r) : l(_l), r(_r) {}
    int size() { return 1; }
    void asm_text()
    {
      C_OPTS::asm_ostr() << "  in   " << C_OPTS::regname(l) << ", ";
      C_OPTS::asm_ostr() << C_OPTS::regname(r) << std::endl;
    }
    void code(int& addr)
    {
      C_OPTS::code_ostr() << "\t" << to_string(addr++) << "    :   ";
      C_OPTS::code_ostr() << to_string(InstrInfo<IN_R_R>::opcode() | l << image.LLO | r << image.RLO);
      C_OPTS::code_ostr() << ";--in_r_r\t";
      C_OPTS::code_ostr() << to_string(InstrInfo<IN_R_R>::opcode()) << " ";
      C_OPTS::code_ostr() << to_string(l) << " " << to_string(r) << std::endl;
    }
    int bytes(int)
    { return InstrInfo<IN_R_R>::opcode() | l << image.LLO | r << image.RLO; }
  };
  
  struct OUT_R_R : Instuction
  {
    int l, r;
    OUT_R_R(int _l, int _r) : l(_l), r(_r) {}
    int size() { return 1; }
    void asm_text()
    {
      C_OPTS::asm_ostr() << "  out  " << C_OPTS::regname(l) << ", ";
      C_OPTS::asm_ostr() << C_OPTS::regname(r) << std::endl;
    }
    void code(int& addr)
    {
      C_OPTS::code_ostr() << "\t" << to_string(addr++) << "    :   ";
      C_OPTS::code_ostr() << to_string(InstrInfo<OUT_R_R>::opcode() | l << image.LLO | r << image.RLO);
      C_OPTS::code_ostr() << ";--out_r_r\t";
      C_OPTS::code_ostr() << to_string(InstrInfo<OUT_R_R>::opcode()) << " ";
      C_OPTS::code_ostr() << to_string(l) << " " << to_string(r) << std::endl;
    }
    int bytes(int)
    { return InstrInfo<OUT_R_R>::opcode() | l << image.LLO | r << image.RLO; }
  };
  
  struct INC_R : Instuction
  {
    int reg;
    INC_R(int _reg) : reg(_reg) {}
    int size() { return 1; }
    void asm_text()
    {
      C_OPTS::asm_ostr() << "  inc  " << C_OPTS::regname(reg);
      C_OPTS::asm_ostr() << std::endl;
    }
    void code(int& addr)
    {
      C_OPTS::code_ostr() << "\t" << to_string(addr++) << "    :   ";
      C_OPTS::code_ostr() << to_string(InstrInfo<INC_R>::opcode() | reg << image.LLO);
      C_OPTS::code_ostr() << ";--inc_r\t";
      C_OPTS::code_ostr() << to_string(InstrInfo<INC_R>::opcode()) << " ";
      C_OPTS::code_ostr() << to_string(reg) << std::endl;
    }
    int bytes(int)
    { return InstrInfo<INC_R>::opcode() | reg << image.LLO; }
  };
  
  struct DEC_R : Instuction
  {
    int reg;
    DEC_R(int _reg) : reg(_reg) {}
    int size() { return 1; }
    void asm_text()
    {
      C_OPTS::asm_ostr() << "  dec  " << C_OPTS::regname(reg);
      C_OPTS::asm_ostr() << std::endl;
    }
    void code(int& addr)
    {
      C_OPTS::code_ostr() << "\t" << to_string(addr++) << "    :   ";
      C_OPTS::code_ostr() << to_string(InstrInfo<DEC_R>::opcode() | reg << image.LLO);
      C_OPTS::code_ostr() << ";--dec_r\t";
      C_OPTS::code_ostr() << to_string(InstrInfo<DEC_R>::opcode()) << " ";
      C_OPTS::code_ostr() << to_string(reg) << std::endl;
    }
    int bytes(int)
    { return InstrInfo<DEC_R>::opcode() | reg << image.LLO; }
  };
  
  public:
  template <class T>
  static void comment(const T& text)
  {
    image.template push_instruction<Comment>(text);
  }
  
  static void mov_r_r(int l, int r)
  {
    image.template push_instruction<MOV_R_R>(l, r);
  }
  
  static void mov_r_b(int reg, int value)
  {
    image.template push_instruction<MOV_R_B>(reg, value);
  }
  
  static void mov_r_static_var_address(int reg, int rel_address)
  {
    image.template push_instruction<MOV_R_STATIC_VAR_ADDRESS>(reg, rel_address);
  }
  
  static void mov_r_a(int l, int r)
  {
    image.template push_instruction<MOV_R_A>(l, r);
  }
  
  static void mov_a_r(int l, int r)
  {
    image.template push_instruction<MOV_A_R>(l, r);
  }
  
  static void push_r(int reg)
  {
    image.template push_instruction<PUSH_R>(reg);
  }
  
  static void pop_r(int reg)
  {
    image.template push_instruction<POP_R>(reg);
  }
  
  static void mov_r_s(int reg, int offset)
  {
    image.template push_instruction<MOV_R_S>(reg, offset);
  }
  
  static void mov_s_r(int offset, int reg)
  {
    image.template push_instruction<MOV_S_R>(offset, reg);
  }
  
  static void sp_to_r(int reg, int offset)
  {
    image.template push_instruction<SP_TO_R>(reg, offset);
  }
  
  static void label(int label_num)
  {
    image.template push_instruction<LABEL>(label_num);
    image.label_address[label_num] = image.size;
  }
  
  static void jmp_label(int label_num)
  {
    image.template push_instruction<JMP_LABEL>(label_num);
  }
  
  static void jeq_label(int label_num)
  {
    image.template push_instruction<JEQ_LABEL>(label_num);
  }
  
  static void jne_label(int label_num)
  {
    image.template push_instruction<JNE_LABEL>(label_num);
  }
  
  static void jle_label(int label_num)
  {
    image.template push_instruction<JLE_LABEL>(label_num);
  }
  
  static void jgr_label(int label_num)
  {
    image.template push_instruction<JGR_LABEL>(label_num);
  }
  
  static void jnl_label(int label_num)
  {
    image.template push_instruction<JNL_LABEL>(label_num);
  }
  
  static void jng_label(int label_num)
  {
    image.template push_instruction<JNG_LABEL>(label_num);
  }
  
  static void call_label(int label_num)
  {
    image.template push_instruction<CALL_LABEL>(label_num);
  }
  
  static void ret()
  {
    image.template push_instruction<RET>();
  }
  
  static void cmp_r_r(int l, int r)
  {
    image.template push_instruction<CMP_R_R>(l, r);
  }
  
  static void or_r_r(int l, int r)
  {
    image.template push_instruction<OR_R_R>(l, r);
  }
  
  static void and_r_r(int l, int r)
  {
    image.template push_instruction<AND_R_R>(l, r);
  }
  
  static void xor_r_r(int l, int r)
  {
    image.template push_instruction<XOR_R_R>(l, r);
  }
  
  static void add_r_r(int l, int r)
  {
    image.template push_instruction<ADD_R_R>(l, r);
  }
  
  static void sub_r_r(int l, int r)
  {
    image.template push_instruction<SUB_R_R>(l, r);
  }
  
  static void mul_r_r(int l, int r)
  {
    image.template push_instruction<MUL_R_R>(l, r);
  }
  
  static void div_r_r(int l, int r)
  {
    image.template push_instruction<DIV_R_R>(l, r);
  }
  
  static void mod_r_r(int l, int r)
  {
    image.template push_instruction<MOD_R_R>(l, r);
  }
  
  static void ls_r(int reg)
  {
    image.template push_instruction<LS_R>(reg);
  }
  
  static void rs_r(int reg)
  {
    image.template push_instruction<RS_R>(reg);
  }
  
  static void ls_r_r(int l, int r)
  {
    image.template push_instruction<LS_R_R>(l, r);
  }
  
  static void rs_r_r(int l, int r)
  {
    image.template push_instruction<RS_R_R>(l, r);
  }
  
  static void in_r_r(int l, int r)
  {
    image.template push_instruction<IN_R_R>(l, r);
  }
  
  static void out_r_r(int l, int r)
  {
    image.template push_instruction<OUT_R_R>(l, r);
  }
  
  static void inc_ar(int reg)
  {
    image.template push_instruction<INC_R>(reg);
  }
  
  static void dec_r(int reg)
  {
    image.template push_instruction<DEC_R>(reg);
  }
  
  static int create_label()
  {
    image.label_address.push_back(0);
    return image.label_address.size() - 1;
  }
  
  static void label()
  {
    image.template push_instruction<LABEL>(create_label());
  }
  
  static int last_label()
  {
    return image.label_address.size() - 1;
  }
  
  static int create_static_var()
  {
    return image.static_vars_counter++;
  }
};

template <class C_OPTS>
typename SomeASM<C_OPTS>::Image SomeASM<C_OPTS>::image;

struct DefaultCOpts
{
  static const int BITS    = 16;
  static const int REGS    = 2;
  static const int RAMSIZE = 512;
  
  static const char* regname(int i)
  {
    static const char* names[] = {"a", "b", "c", "d"};
    return names[i];
  }
};