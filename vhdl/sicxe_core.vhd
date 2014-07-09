library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_unsigned.all;
use ieee.numeric_std.all;

entity sicxe_core is
  Port (
    clock_i : in std_logic;
    reset_i : in std_logic;
    enable_i : in std_logic;
    error_o : out std_logic;
    -- memory
    memory_read_o : out std_logic;
    memory_write_o : out std_logic;
    memory_address_o : out std_logic_vector(19 downto 0);
    memory_data_in_i : in std_logic_vector(7 downto 0);
    memory_data_out_o : out std_logic_vector(7 downto 0);
    memory_done_i : in std_logic;
    -- device ports
    port_id_o : out std_logic_vector(7 downto 0);
    port_in_i : in std_logic_vector(7 downto 0);
    port_out_o : out std_logic_vector(7 downto 0);
    port_read_strobe_o : out std_logic;
    port_write_strobe_o : out std_logic;
    -- interrupt
    interrupt_i : in std_logic;
    interrupt_acknowledge_o : out std_logic;
    interrupt_enabled_o : out std_logic
  );
end sicxe_core;

architecture behavioral of sicxe_core is
  -- opcodes
  constant OPCODE_LONG_EINT : std_logic_vector := "11111000";
  constant OPCODE_LONG_DINT : std_logic_vector := "11111001";
  constant OPCODE_LONG_RINT : std_logic_vector := "11111010";

  constant OPCODE_LONG_CLEAR : std_logic_vector := "10110100";
  constant OPCODE_LONG_RMO : std_logic_vector := "10101100";
  constant OPCODE_LONG_ADDR : std_logic_vector := "10010000";
  constant OPCODE_LONG_SUBR : std_logic_vector := "10010100";
  constant OPCODE_LONG_MULR : std_logic_vector := "10011000";
  constant OPCODE_LONG_SHIFTL : std_logic_vector := "10100100";
  constant OPCODE_LONG_SHIFTR : std_logic_vector := "10101000";
  constant OPCODE_LONG_COMPR : std_logic_vector := "10100000";
  constant OPCODE_LONG_TIXR : std_logic_vector := "10111000";
  constant OPCODE_LONG_ANDR : std_logic_vector := "11110100";
  constant OPCODE_LONG_ORR : std_logic_vector := "11110101";
  constant OPCODE_LONG_XORR : std_logic_vector := "11110110";
  constant OPCODE_LONG_NOT : std_logic_vector := "11110111";

  constant OPCODE_SHORT_ADD : std_logic_vector := "000110";
  constant OPCODE_SHORT_SUB : std_logic_vector := "000111";
  constant OPCODE_SHORT_MUL : std_logic_vector := "001000";
  constant OPCODE_SHORT_AND : std_logic_vector := "010000";
  constant OPCODE_SHORT_OR : std_logic_vector := "010001";
  constant OPCODE_SHORT_COMP : std_logic_vector := "001010";
  constant OPCODE_SHORT_TIX : std_logic_vector := "001011";
  constant OPCODE_SHORT_J : std_logic_vector := "001111";
  constant OPCODE_SHORT_JEQ : std_logic_vector := "001100";
  constant OPCODE_SHORT_JGT : std_logic_vector := "001101";
  constant OPCODE_SHORT_JLT : std_logic_vector := "001110";
  constant OPCODE_SHORT_JSUB : std_logic_vector := "010010";
  constant OPCODE_SHORT_RSUB : std_logic_vector := "010011";
  constant OPCODE_SHORT_LDCH : std_logic_vector := "010100";
  constant OPCODE_SHORT_LDA : std_logic_vector := "000000";
  constant OPCODE_SHORT_LDB : std_logic_vector := "011010";
  constant OPCODE_SHORT_LDL : std_logic_vector := "000010";
  constant OPCODE_SHORT_LDS : std_logic_vector := "011011";
  constant OPCODE_SHORT_LDT : std_logic_vector := "011101";
  constant OPCODE_SHORT_LDX : std_logic_vector := "000001";
  constant OPCODE_SHORT_STCH : std_logic_vector := "010101";
  constant OPCODE_SHORT_STSW : std_logic_vector := "111010";
  constant OPCODE_SHORT_STA : std_logic_vector := "000011";
  constant OPCODE_SHORT_STB : std_logic_vector := "011110";
  constant OPCODE_SHORT_STL : std_logic_vector := "000101";
  constant OPCODE_SHORT_STS : std_logic_vector := "011111";
  constant OPCODE_SHORT_STT : std_logic_vector := "100001";
  constant OPCODE_SHORT_STX : std_logic_vector := "000100";
  constant OPCODE_SHORT_TD : std_logic_vector := "111000";
  constant OPCODE_SHORT_RD : std_logic_vector := "110110";
  constant OPCODE_SHORT_WD : std_logic_vector := "110111";
  constant OPCODE_SHORT_XOR : std_logic_vector := "111100";
  constant OPCODE_SHORT_STIL : std_logic_vector := "111111";

  -- ALU
  type alu_operation_type is (ALU_ZERO, ALU_PASS1, ALU_PASS2, ALU_ADD, ALU_SUB,
                              ALU_MUL, ALU_AND, ALU_OR, ALU_XOR, ALU_NOT,
                              ALU_SHIFTL, ALU_SHIFTR);
  signal alu_operation : alu_operation_type;
  signal alu_operand1 : std_logic_vector(23 downto 0);
  signal alu_operand2 : std_logic_vector(23 downto 0);
  signal alu_shift_bits : std_logic_vector(3 downto 0);
  signal alu_result : std_logic_vector(23 downto 0);
  signal alu_compare_result_left : std_logic_vector(1 downto 0);
  signal alu_compare_result_right : std_logic_vector(1 downto 0);

  -- general registers
  signal reg_general_a : std_logic_vector(23 downto 0);
  signal reg_general_x : std_logic_vector(23 downto 0);
  signal reg_general_l : std_logic_vector(23 downto 0);
  signal reg_general_b : std_logic_vector(23 downto 0);
  signal reg_general_s : std_logic_vector(23 downto 0);
  signal reg_general_t : std_logic_vector(23 downto 0);

  signal reg_general_write : std_logic;

  signal reg_general_select : std_logic_vector(3 downto 0);
  signal reg_general_select_write : std_logic;
  type select_general_type is (SELECT_GENERAL_A, SELECT_GENERAL_X, SELECT_GENERAL_L,
                               SELECT_GENERAL_R1, SELECT_GENERAL_R2,
                               SELECT_GENERAL_LOAD_INSN);
  signal select_general : select_general_type;


  -- operand registers
  signal reg_operand1 : std_logic_vector(23 downto 0);
  signal reg_operand2 : std_logic_vector(23 downto 0);
  signal reg_operand3 : std_logic_vector(23 downto 0);

  signal reg_operand1_write : std_logic;
  signal reg_operand2_write : std_logic;
  signal reg_operand3_write : std_logic;


  -- result register
  signal reg_result : std_logic_vector(23 downto 0);

  signal reg_result_write : std_logic;


  -- special registers
  signal reg_special_target : std_logic_vector(23 downto 0);
  signal reg_special_pc : std_logic_vector(19 downto 0);
  signal reg_special_il : std_logic_vector(19 downto 0);
  signal reg_special_cc : std_logic_vector(1 downto 0);
  signal reg_special_icc : std_logic_vector(1 downto 0);

  signal reg_special_target_write : std_logic;
  signal reg_special_pc_write : std_logic;
  signal reg_special_pc_write_cond : std_logic;
  signal reg_special_il_write : std_logic;
  signal reg_special_cc_clear : std_logic;
  signal reg_special_cc_write_left : std_logic;
  signal reg_special_cc_write_right : std_logic;
  signal reg_special_cc_save : std_logic;
  signal reg_special_cc_restore : std_logic;

  -- conditional PC write
  signal reg_special_pc_write_cond_lt : std_logic;
  signal reg_special_pc_write_cond_eq : std_logic;
  signal reg_special_pc_write_cond_gt : std_logic;


  -- interrupt enable register
  signal reg_interrupt : std_logic;
  signal reg_interrupt_next : std_logic;

  signal interrupt_disable : std_logic;
  signal interrupt_enable : std_logic;
  signal interrupt_move : std_logic;


  -- memory data register
  signal reg_memory_data : std_logic_vector(23 downto 0);

  signal reg_memory_data_write_result : std_logic;
  signal reg_memory_data_write_mem : std_logic_vector(2 downto 0);


  -- device data register
  signal reg_device_data : std_logic_vector(7 downto 0);

  signal reg_device_data_write_result : std_logic;
  signal reg_device_data_write_dev : std_logic;


  -- instruction register
  signal reg_instruction : std_logic_vector(31 downto 0);

  signal reg_instruction_write : std_logic_vector(3 downto 0);

  signal insn_opcode : std_logic_vector(7 downto 0);
  signal insn_flag_n : std_logic;
  signal insn_flag_i : std_logic;
  signal insn_flag_x : std_logic;
  signal insn_flag_b : std_logic;
  signal insn_flag_p : std_logic;
  signal insn_flag_e : std_logic;
  signal insn_r1 : std_logic_vector(3 downto 0);
  signal insn_r2 : std_logic_vector(3 downto 0);
  signal insn_operand_f3_usgn : std_logic_vector(23 downto 0);
  signal insn_operand_f3_sgn : std_logic_vector(23 downto 0);
  signal insn_operand_f4_usgn : std_logic_vector(23 downto 0);
  signal insn_operand_sic : std_logic_vector(23 downto 0);

  signal insn_r1_valid : std_logic;
  signal insn_r2_valid : std_logic;

  -- operand select
  type select_op1_type is (SELECT_OP1_ROP1, SELECT_OP1_X, SELECT_OP1_TARGET,
                           SELECT_OP1_PC, SELECT_OP1_IL, SELECT_OP1_MEM,
                           SELECT_OP1_MEM_BYTE, SELECT_OP1_DEV, SELECT_OP1_F3USGN,
                           SELECT_OP1_F3SGN, SELECT_OP1_F4USGN, SELECT_OP1_SIC);
  signal select_op1 : select_op1_type;

  type select_op2_type is (SELECT_OP2_CONE, SELECT_OP2_CIV, SELECT_OP2_ROP2,
                           SELECT_OP2_ROP3, SELECT_OP2_A, SELECT_OP2_X, SELECT_OP2_B,
                           SELECT_OP2_L, SELECT_OP2_PC, SELECT_OP2_SW);

  signal select_op2 : select_op2_type;


  -- memory address select
  type select_addr_type is (SELECT_ADDR_PC, SELECT_ADDR_TARGET);
  signal select_addr : select_addr_type;


  -- memory out data select
  type select_mem_type is (SELECT_MEM_BYTE0, SELECT_MEM_BYTE1, SELECT_MEM_BYTE2);
  signal select_mem : select_mem_type;


  -- control unit FSM
  type ctl_state_type is (CTL_ERROR, CTL_DISABLED, CTL_INSN0, CTL_DECODE0,
                          CTL_F1_EINT, CTL_F1_DINT, CTL_F1_RINT,
                          CTL_F2_INSN1, CTL_F2_DECODE1, CTL_F2_ALU0, CTL_F2_ALU1,
                          CTL_F2_COMP, CTL_F2_TIX0, CTL_F2_TIX1, CTL_F2_TIX2,
                          CTL_F34_INSN1, CTL_F34_INSN2, CTL_F34_INSN3, CTL_F34_DECODE1,
                          CTL_F34_DECODE2, CTL_F34_INDEXED,
                          CTL_F34_INDIRECT0, CTL_F34_INDIRECT1, CTL_F34_INDIRECT2,
                          CTL_F34_INDIRECT3, CTL_F34_RSUB,
                          CTL_F34B_LOAD0, CTL_F34B_LOAD1, CTL_F34B_DECODE3, CTL_F34B_WD0,
                          CTL_F34B_WD1, CTL_F34B_WD2, CTL_F34B_TD, CTL_F34B_RD0,
                          CTL_F34B_RD1, CTL_F34B_RD2, CTL_F34B_RD3, CTL_F34B_LDCH0,
                          CTL_F34B_LDCH1,
                          CTL_F34B_STSW, CTL_F34B_STCH,
                          CTL_F34W_STR0, CTL_F34W_STR1, CTL_F34W_STIL,
                          CTL_F34_STORE0, CTL_F34_STORE1, CTL_F34_STORE2,
                          CTL_F34_JUMP, CTL_F34_JSUB0, CTL_F34_JSUB1,
                          CTL_F34W_LOAD0, CTL_F34W_LOAD1, CTL_F34W_LOAD2, CTL_F34W_LOAD3,
                          CTL_F34W_DECODE3, CTL_F34W_LDR0, CTL_F34W_LDR1,
                          CTL_F34W_COMP, CTL_F34W_TIX0, CTL_F34W_TIX1, CTL_F34W_TIX2,
                          CTL_F34W_ALU0, CTL_F34W_ALU1,
                          CTL_INT0, CTL_INT1, CTL_INT2, CTL_INT3, CTL_INT4, CTL_INT5);
  signal ctl_state : ctl_state_type;
  signal ctl_next_state : ctl_state_type;

begin
  -- ALU
  alu_proc : process(alu_operation, alu_operand1, alu_operand2, alu_shift_bits)
  begin
    alu_result <= (others => '0');
    case (alu_operation) is
      when ALU_ZERO =>
        alu_result <= (others => '0');
      when ALU_PASS1 =>
        alu_result <= alu_operand1;
      when ALU_PASS2 =>
        alu_result <= alu_operand2;
      when ALU_ADD =>
        alu_result <= std_logic_vector(signed(alu_operand2) + signed(alu_operand1));
      when ALU_SUB =>
        alu_result <= std_logic_vector(signed(alu_operand2) - signed(alu_operand1));
      when ALU_MUL =>
        alu_result <= std_logic_vector(
                          resize(signed(alu_operand2) * signed(alu_operand1), 24));
      when ALU_AND =>
        alu_result <= std_logic_vector(signed(alu_operand2) and signed(alu_operand1));
      when ALU_OR =>
        alu_result <= std_logic_vector(signed(alu_operand2) or signed(alu_operand1));
      when ALU_XOR =>
        alu_result <= std_logic_vector(signed(alu_operand2) xor signed(alu_operand1));
      when ALU_NOT =>
        alu_result <= std_logic_vector(not signed(alu_operand1));
      when ALU_SHIFTL =>
        alu_result <= std_logic_vector(
                          signed(alu_operand1) sll to_integer(unsigned(alu_shift_bits)));
      when ALU_SHIFTR =>
        alu_result <= std_logic_vector(shift_right(signed(alu_operand1),
                          to_integer(unsigned(alu_shift_bits))));
      when others =>
    end case;
  end process;

  alu_compare_proc : process(alu_operand1, alu_operand2)
  begin
    if (signed(alu_operand1) < signed(alu_operand2)) then
      alu_compare_result_left <= "00";
      alu_compare_result_right <= "10";
    elsif (signed(alu_operand1) = signed(alu_operand2)) then
      alu_compare_result_left <= "01";
      alu_compare_result_right <= "01";
    else
      alu_compare_result_left <= "10";
      alu_compare_result_right <= "00";
    end if;
  end process;

  -- general registers
  reg_general_proc : process(clock_i)
  begin
    if (rising_edge(clock_i)) then
      if (reset_i = '1') then
        reg_general_a <= (others => '0');
        reg_general_x <= (others => '0');
        reg_general_l <= (others => '0');
        reg_general_b <= (others => '0');
        reg_general_s <= (others => '0');
        reg_general_t <= (others => '0');
      else
        reg_general_a <= reg_general_a;
        reg_general_x <= reg_general_x;
        reg_general_l <= reg_general_l;
        reg_general_b <= reg_general_b;
        reg_general_s <= reg_general_s;
        reg_general_t <= reg_general_t;
        if (reg_general_write = '1') then
          case (reg_general_select) is
            when x"0" =>
              reg_general_a <= reg_result;
            when x"1" =>
              reg_general_x <= reg_result;
            when x"2" =>
              reg_general_l <= reg_result;
            when x"3" =>
              reg_general_b <= reg_result;
            when x"4" =>
              reg_general_s <= reg_result;
            when x"5" =>
              reg_general_t <= reg_result;
            when others =>
          end case;
        end if;
      end if;
    end if;
  end process;

  reg_general_select_proc : process(clock_i)
  begin
    if (rising_edge(clock_i)) then
      if (reset_i = '1') then
        reg_general_select <= x"0";
      else
        reg_general_select <= reg_general_select;
        if (reg_general_select_write = '1') then
          case (select_general) is
            when SELECT_GENERAL_A =>
              reg_general_select <= x"0";
            when SELECT_GENERAL_X =>
              reg_general_select <= x"1";
            when SELECT_GENERAL_L =>
              reg_general_select <= x"2";
            when SELECT_GENERAL_R1 =>
              reg_general_select <= insn_r1;
            when SELECT_GENERAL_R2 =>
              reg_general_select <= insn_r2;
            when SELECT_GENERAL_LOAD_INSN =>
              case (insn_opcode(7 downto 2)) is
                when OPCODE_SHORT_LDA =>
                  reg_general_select <= x"0";
                when OPCODE_SHORT_LDX =>
                  reg_general_select <= x"1";
                when OPCODE_SHORT_LDL =>
                  reg_general_select <= x"2";
                when OPCODE_SHORT_LDB =>
                  reg_general_select <= x"3";
                when OPCODE_SHORT_LDS =>
                  reg_general_select <= x"4";
                when OPCODE_SHORT_LDT =>
                  reg_general_select <= x"5";
                when others =>
                  reg_general_select <= x"0";
              end case;
            when others =>
          end case;
        end if;
      end if;
    end if;
  end process;

  -- special registers
  reg_special_proc : process(clock_i)
  begin
    if (rising_edge(clock_i)) then
      if (reset_i = '1') then
        reg_special_target <= (others => '0');
        reg_special_pc <= (others => '0');
        reg_special_il <= (others => '0');
        reg_special_cc <= (others => '0');
        reg_special_icc <= (others => '0');
      else
        reg_special_target <= reg_special_target;
        reg_special_pc <= reg_special_pc;
        reg_special_il <= reg_special_il;
        reg_special_cc <= reg_special_cc;
        reg_special_icc <= reg_special_icc;
        if (reg_special_target_write = '1') then
          reg_special_target <= alu_result;
        end if;
        if (reg_special_pc_write = '1' or reg_special_pc_write_cond = '1') then
          reg_special_pc <= alu_result(19 downto 0);
        end if;
        if (reg_special_il_write = '1') then
          reg_special_il <= alu_result(19 downto 0);
        end if;
        if (reg_special_cc_clear = '1') then
          reg_special_cc <= (others => '0');
        elsif (reg_special_cc_write_left = '1') then
          reg_special_cc <= alu_compare_result_left;
        elsif (reg_special_cc_write_right = '1') then
          reg_special_cc <= alu_compare_result_right;
        elsif (reg_special_cc_restore = '1') then
          reg_special_cc <= reg_special_icc;
        end if;
        if (reg_special_cc_save = '1') then
          reg_special_icc <= reg_special_cc;
        end if;
      end if;
    end if;
  end process;

  -- conditional PC write
  reg_special_pc_write_cond_proc : process(reg_special_cc,
                                           reg_special_pc_write_cond_lt,
                                           reg_special_pc_write_cond_eq,
                                           reg_special_pc_write_cond_gt)
  begin
    reg_special_pc_write_cond <= '0';
    case (reg_special_cc) is
      when "00" =>
        reg_special_pc_write_cond <= reg_special_pc_write_cond_lt;
      when "01" =>
        reg_special_pc_write_cond <= reg_special_pc_write_cond_eq;
      when "10" =>
        reg_special_pc_write_cond <= reg_special_pc_write_cond_gt;
      when others =>
    end case;
  end process;

  -- operand registers
  reg_operand_proc : process(clock_i)
  begin
    if (rising_edge(clock_i)) then
      if (reset_i = '1') then
        reg_operand1 <= (others => '0');
        reg_operand2 <= (others => '0');
        reg_operand3 <= (others => '0');
      else
        reg_operand1 <= reg_operand1;
        reg_operand2 <= reg_operand2;
        reg_operand3 <= reg_operand3;
        if (reg_operand1_write = '1') then
          case (insn_r1) is
            when x"0" =>
              reg_operand1 <= reg_general_a;
            when x"1" =>
              reg_operand1 <= reg_general_x;
            when x"2" =>
              reg_operand1 <= reg_general_l;
            when x"3" =>
              reg_operand1 <= reg_general_b;
            when x"4" =>
              reg_operand1 <= reg_general_s;
            when x"5" =>
              reg_operand1 <= reg_general_t;
            when others =>
          end case;
        end if;
        if (reg_operand2_write = '1') then
          case (insn_r2) is
            when x"0" =>
              reg_operand2 <= reg_general_a;
            when x"1" =>
              reg_operand2 <= reg_general_x;
            when x"2" =>
              reg_operand2 <= reg_general_l;
            when x"3" =>
              reg_operand2 <= reg_general_b;
            when x"4" =>
              reg_operand2 <= reg_general_s;
            when x"5" =>
              reg_operand2 <= reg_general_t;
            when others =>
          end case;
        end if;
        if (reg_operand3_write = '1') then
          case (insn_opcode(7 downto 2)) is
            when OPCODE_SHORT_STA =>
              reg_operand3 <= reg_general_a;
            when OPCODE_SHORT_STX =>
              reg_operand3 <= reg_general_x;
            when OPCODE_SHORT_STL =>
              reg_operand3 <= reg_general_l;
            when OPCODE_SHORT_STB =>
              reg_operand3 <= reg_general_b;
            when OPCODE_SHORT_STS =>
              reg_operand3 <= reg_general_s;
            when OPCODE_SHORT_STT =>
              reg_operand3 <= reg_general_t;
            when others =>
          end case;
        end if;
      end if;
    end if;
  end process;

  -- result register
  reg_result_proc : process(clock_i)
  begin
    if (rising_edge(clock_i)) then
      if (reset_i = '1') then
        reg_result <= (others => '0');
      else
        if (reg_result_write = '1') then
          reg_result <= alu_result;
        else
          reg_result <= reg_result;
        end if;
      end if;
    end if;
  end process;

  -- interrupt enable register
  reg_interrupt_proc : process(clock_i)
  begin
    if (rising_edge(clock_i)) then
      if (reset_i = '1') then
        reg_interrupt <= '0';
        reg_interrupt_next <= '0';
      else
        if (interrupt_disable = '1') then
          reg_interrupt <= '0';
          reg_interrupt_next <= '0';
        else
          if (interrupt_move = '1') then
            reg_interrupt <= reg_interrupt_next;
          else
            reg_interrupt <= reg_interrupt;
          end if;
          if (interrupt_enable = '1') then
            reg_interrupt_next <= '1';
          else
            reg_interrupt_next <= reg_interrupt_next;
          end if;
        end if;
      end if;
    end if;
  end process;

  -- memory data register
  reg_memory_proc : process(clock_i)
  begin
    if (rising_edge(clock_i)) then
      if (reset_i = '1') then
        reg_memory_data <= (others => '0');
      else
        if (reg_memory_data_write_result = '1') then
          reg_memory_data <= alu_result;
        else
          reg_memory_data <= reg_memory_data;
          if (reg_memory_data_write_mem(0) = '1') then
            reg_memory_data(7 downto 0) <= memory_data_in_i;
          end if;
          if (reg_memory_data_write_mem(1) = '1') then
            reg_memory_data(15 downto 8) <= memory_data_in_i;
          end if;
          if (reg_memory_data_write_mem(2) = '1') then
            reg_memory_data(23 downto 16) <= memory_data_in_i;
          end if;
        end if;
      end if;
    end if;
  end process;

  -- device data register
  reg_device_proc : process(clock_i)
  begin
    if (rising_edge(clock_i)) then
      if (reset_i = '1') then
        reg_device_data <= (others => '0');
      else
        if (reg_device_data_write_result = '1') then
          reg_device_data <= alu_result(7 downto 0);
        elsif (reg_device_data_write_dev = '1') then
          reg_device_data <= port_in_i;
        else
          reg_device_data <= reg_device_data;
        end if;
      end if;
    end if;
  end process;

  -- instruction register
  reg_instruction_proc : process(clock_i)
  begin
    if (rising_edge(clock_i)) then
      if (reset_i = '1') then
        reg_instruction <= (others => '0');
      else
        reg_instruction <= reg_instruction;
        if (reg_instruction_write(0) = '1') then
          reg_instruction(7 downto 0) <= memory_data_in_i;
        end if;
        if (reg_instruction_write(1) = '1') then
          reg_instruction(15 downto 8) <= memory_data_in_i;
        end if;
        if (reg_instruction_write(2) = '1') then
          reg_instruction(23 downto 16) <= memory_data_in_i;
        end if;
        if (reg_instruction_write(3) = '1') then
          reg_instruction(31 downto 24) <= memory_data_in_i;
        end if;
      end if;
    end if;
  end process;

  insn_opcode <= reg_instruction(31 downto 24);
  insn_flag_n <= reg_instruction(25);
  insn_flag_i <= reg_instruction(24);
  insn_flag_x <= reg_instruction(23);
  insn_flag_b <= reg_instruction(22);
  insn_flag_p <= reg_instruction(21);
  insn_flag_e <= reg_instruction(20);
  insn_r1 <= reg_instruction(23 downto 20);
  insn_r2 <= reg_instruction(19 downto 16);
  insn_operand_f3_usgn <= std_logic_vector(
                              resize(unsigned(reg_instruction(19 downto 8)), 24));
  insn_operand_f3_sgn <= std_logic_vector(
                             resize(signed(reg_instruction(19 downto 8)), 24));
  insn_operand_f4_usgn <= std_logic_vector(
                              resize(unsigned(reg_instruction(19 downto 0)), 24));
  insn_operand_sic <= std_logic_vector(
                          resize(unsigned(reg_instruction(22 downto 8)), 24));

  insn_r1_valid_proc : process(insn_r1)
  begin
    case (insn_r1) is
      when x"0" | x"1" | x"2" | x"3" | x"4" | x"5" =>
        insn_r1_valid <= '1';
      when others =>
        insn_r1_valid <= '0';
    end case;
  end process;

  insn_r2_valid_proc : process(insn_r2)
  begin
    case (insn_r2) is
      when x"0" | x"1" | x"2" | x"3" | x"4" | x"5" =>
        insn_r2_valid <= '1';
      when others =>
        insn_r2_valid <= '0';
    end case;
  end process;

  -- operand select
  select_op1_proc : process(select_op1, reg_operand1, reg_general_x,
                            reg_special_target, reg_special_pc, reg_special_il,
                            reg_memory_data, reg_device_data, insn_operand_f3_usgn,
                            insn_operand_f3_sgn, insn_operand_f4_usgn, insn_operand_sic)
  begin
    case (select_op1) is
      when SELECT_OP1_ROP1 =>
        alu_operand1 <= reg_operand1;
      when SELECT_OP1_X =>
        alu_operand1 <= reg_general_x;
      when SELECT_OP1_TARGET =>
        alu_operand1 <= reg_special_target;
      when SELECT_OP1_PC =>
        alu_operand1 <= "0000" & reg_special_pc;
      when SELECT_OP1_IL =>
        alu_operand1 <= "0000" & reg_special_il;
      when SELECT_OP1_MEM =>
        alu_operand1 <= reg_memory_data;
      when SELECT_OP1_MEM_BYTE =>
        alu_operand1 <= x"0000" & reg_memory_data(7 downto 0);
      when SELECT_OP1_DEV =>
        alu_operand1 <= x"0000" & reg_device_data;
      when SELECT_OP1_F3USGN =>
        alu_operand1 <= insn_operand_f3_usgn;
      when SELECT_OP1_F3SGN =>
        alu_operand1 <= insn_operand_f3_sgn;
      when SELECT_OP1_F4USGN =>
        alu_operand1 <= insn_operand_f4_usgn;
      when SELECT_OP1_SIC =>
        alu_operand1 <= insn_operand_sic;
      when others =>
        alu_operand1 <= (others => '0');
    end case;
  end process;

  select_op2_proc : process(select_op2, reg_operand2, reg_operand3, reg_general_a,
                            reg_general_x, reg_general_b, reg_general_l,
                            reg_special_pc, reg_interrupt, reg_special_icc,
                            reg_special_cc)
  begin
    case (select_op2) is
      when SELECT_OP2_CONE =>
        alu_operand2 <= x"000001";
      when SELECT_OP2_CIV =>
        alu_operand2 <= x"0ffffd";
      when SELECT_OP2_ROP2 =>
        alu_operand2 <= reg_operand2;
      when SELECT_OP2_ROP3 =>
        alu_operand2 <= reg_operand3;
      when SELECT_OP2_A =>
        alu_operand2 <= reg_general_a;
      when SELECT_OP2_X =>
        alu_operand2 <= reg_general_x;
      when SELECT_OP2_B =>
        alu_operand2 <= reg_general_b;
      when SELECT_OP2_L =>
        alu_operand2 <= reg_general_l;
      when SELECT_OP2_PC =>
        alu_operand2 <= "0000" & reg_special_pc;
      when SELECT_OP2_SW =>
        alu_operand2 <= x"0000" & "000" &
                            reg_interrupt & reg_special_icc & reg_special_cc;
      when others =>
        alu_operand2 <= (others => '0');
    end case;
  end process;

  -- memory address select
  select_addr_proc : process(select_addr, reg_special_target, reg_special_pc)
  begin
    case (select_addr) is
      when SELECT_ADDR_PC =>
        memory_address_o <= reg_special_pc;
      when SELECT_ADDR_TARGET =>
        memory_address_o <= reg_special_target(19 downto 0);
      when others =>
        memory_address_o <= (others => '0');
    end case;
  end process;

  -- memory out data select
  select_mem_proc : process(select_mem, reg_memory_data)
  begin
    case (select_mem) is
      when SELECT_MEM_BYTE0 =>
        memory_data_out_o <= reg_memory_data(7 downto 0);
      when SELECT_MEM_BYTE1 =>
        memory_data_out_o <= reg_memory_data(15 downto 8);
      when SELECT_MEM_BYTE2 =>
        memory_data_out_o <= reg_memory_data(23 downto 16);
      when others =>
        memory_data_out_o <= (others => '0');
    end case;
  end process;

  -- other connections
  port_id_o <= reg_special_target(7 downto 0);
  port_out_o <= reg_device_data;
  alu_shift_bits <= insn_r2;
  interrupt_enabled_o <= reg_interrupt;

  -- control unit FSM
  ctl_sync_proc : process(clock_i)
  begin
    if (rising_edge(clock_i)) then
      if (reset_i = '1') then
        ctl_state <= CTL_DISABLED;
      else
        ctl_state <= ctl_next_state;
      end if;
    end if;
  end process;

  ctl_state_proc : process(ctl_state, enable_i, interrupt_i, reg_interrupt,
                           insn_opcode, insn_r1_valid, insn_r2_valid,
                           insn_flag_n, insn_flag_i, insn_flag_x, insn_flag_b,
                           insn_flag_p, insn_flag_e, memory_done_i)
  begin
    ctl_next_state <= ctl_state;
    case (ctl_state) is
      -- special states
      when CTL_ERROR =>
      when CTL_DISABLED =>
        if (enable_i = '1') then
          if (reg_interrupt = '1' and interrupt_i = '1') then
            ctl_next_state <= CTL_INT0;
          else
            ctl_next_state <= CTL_INSN0;
          end if;
        end if;

      -- instruction read & decode
      when CTL_INSN0 =>
        if (memory_done_i = '1') then
          ctl_next_state <= CTL_DECODE0;
        end if;
      when CTL_DECODE0 =>
        case (insn_opcode) is
          when OPCODE_LONG_EINT =>
            ctl_next_state <= CTL_F1_EINT;
          when OPCODE_LONG_DINT =>
            ctl_next_state <= CTL_F1_DINT;
          when OPCODE_LONG_RINT =>
            ctl_next_state <= CTL_F1_RINT;
          when OPCODE_LONG_CLEAR | OPCODE_LONG_RMO | OPCODE_LONG_ADDR |
               OPCODE_LONG_SUBR | OPCODE_LONG_MULR | OPCODE_LONG_SHIFTL |
               OPCODE_LONG_SHIFTR  | OPCODE_LONG_COMPR | OPCODE_LONG_TIXR |
               OPCODE_LONG_ANDR | OPCODE_LONG_ORR | OPCODE_LONG_XORR |
               OPCODE_LONG_NOT =>
            ctl_next_state <= CTL_F2_INSN1;
          when others =>
            case (insn_opcode(7 downto 2)) is
              when OPCODE_SHORT_ADD | OPCODE_SHORT_SUB |OPCODE_SHORT_MUL |
                   OPCODE_SHORT_AND |OPCODE_SHORT_OR | OPCODE_SHORT_COMP |
                   OPCODE_SHORT_TIX | OPCODE_SHORT_J | OPCODE_SHORT_JEQ |
                   OPCODE_SHORT_JGT | OPCODE_SHORT_JLT | OPCODE_SHORT_JSUB |
                   OPCODE_SHORT_RSUB | OPCODE_SHORT_LDCH | OPCODE_SHORT_LDA |
                   OPCODE_SHORT_LDB | OPCODE_SHORT_LDL | OPCODE_SHORT_LDS |
                   OPCODE_SHORT_LDT | OPCODE_SHORT_LDX | OPCODE_SHORT_STCH |
                   OPCODE_SHORT_STSW | OPCODE_SHORT_STA | OPCODE_SHORT_STB |
                   OPCODE_SHORT_STL | OPCODE_SHORT_STS | OPCODE_SHORT_STT |
                   OPCODE_SHORT_STX | OPCODE_SHORT_TD | OPCODE_SHORT_RD |
                   OPCODE_SHORT_WD | OPCODE_SHORT_XOR | OPCODE_SHORT_STIL =>
                ctl_next_state <= CTL_F34_INSN1;
              when others =>
              ctl_next_state <= CTL_ERROR;
            end case;
        end case;

      -- format 1 instructions
      when CTL_F1_EINT | CTL_F1_DINT =>
        if (enable_i = '1') then
          ctl_next_state <= CTL_INSN0;
        else
          ctl_next_state <= CTL_DISABLED;
        end if;

      -- format 2 instructions
      when CTL_F2_INSN1 =>
        if (memory_done_i = '1') then
          ctl_next_state <= CTL_F2_DECODE1;
        end if;
      when CTL_F2_DECODE1 =>
        case (insn_opcode) is
          when OPCODE_LONG_CLEAR | OPCODE_LONG_NOT |
               OPCODE_LONG_SHIFTL | OPCODE_LONG_SHIFTR =>
            if (insn_r1_valid = '1') then
              ctl_next_state <= CTL_F2_ALU0;
            else
              ctl_next_state <= CTL_ERROR;
            end if;
          when OPCODE_LONG_COMPR =>
            if (insn_r1_valid = '1' and insn_r2_valid = '1') then
              ctl_next_state <= CTL_F2_COMP;
            else
              ctl_next_state <= CTL_ERROR;
            end if;
          when OPCODE_LONG_TIXR =>
            if (insn_r1_valid = '1') then
              ctl_next_state <= CTL_F2_TIX0;
            else
              ctl_next_state <= CTL_ERROR;
            end if;
          when others =>
            if (insn_r1_valid = '1' and insn_r2_valid = '1') then
              ctl_next_state <= CTL_F2_ALU0;
            else
              ctl_next_state <= CTL_ERROR;
            end if;
        end case;
      when CTL_F2_ALU0 =>
        ctl_next_state <= CTL_F2_ALU1;
      when CTL_F2_TIX0 =>
        ctl_next_state <= CTL_F2_TIX1;
      when CTL_F2_TIX1 =>
        ctl_next_state <= CTL_F2_TIX2;

      -- format S34 instructions
      when CTL_F34_INSN1 =>
        if (memory_done_i = '1') then
          ctl_next_state <= CTL_F34_INSN2;
        end if;
      when CTL_F34_INSN2 =>
        if (memory_done_i = '1') then
          if (insn_flag_e = '1') then
            ctl_next_state <= CTL_F34_INSN3;
          else
            ctl_next_state <= CTL_F34_DECODE1;
          end if;
        end if;
      when CTL_F34_INSN3 =>
        if (memory_done_i = '1') then
          ctl_next_state <= CTL_F34_DECODE1;
        end if;
      when CTL_F34_DECODE1 =>
        if (insn_opcode(7 downto 2) = OPCODE_SHORT_RSUB) then
          ctl_next_state <= CTL_F34_RSUB;
        else
          if (insn_flag_n = '0' and insn_flag_i = '0') then
            -- simple addressing (SIC)
            if (insn_flag_x = '1') then
              ctl_next_state <= CTL_F34_INDEXED;
            else
              ctl_next_state <= CTL_F34_DECODE2;
            end if;
          else
            if ((insn_flag_b = '1' and insn_flag_p = '1') or
                (insn_flag_e = '1' and (insn_flag_b = '1' or insn_flag_p = '1'))) then
              -- invalid addressing
              ctl_next_state <= CTL_ERROR;
            else
              if (insn_flag_n = '1' and insn_flag_i = '1') then
                -- simple addressing
                if (insn_flag_x = '1') then
                  ctl_next_state <= CTL_F34_INDEXED;
                else
                  ctl_next_state <= CTL_F34_DECODE2;
                end if;
              else
                if (insn_flag_x = '1') then
                  -- invalid addressing
                  ctl_next_state <= CTL_ERROR;
                else
                  if (insn_flag_n = '0' and insn_flag_i = '1') then
                    -- immediate addressing
                    ctl_next_state <= CTL_F34_DECODE2;
                  else
                    -- indirect addressing
                    ctl_next_state <= CTL_F34_INDIRECT0;
                  end if;
                end if;
              end if;
            end if;
          end if;
        end if;
      when CTL_F34_INDEXED =>
        ctl_next_state <= CTL_F34_DECODE2;
      when CTL_F34_INDIRECT0 =>
        if (memory_done_i = '1') then
          ctl_next_state <= CTL_F34_INDIRECT1;
        end if;
      when CTL_F34_INDIRECT1 =>
        if (memory_done_i = '1') then
          ctl_next_state <= CTL_F34_INDIRECT2;
        end if;
      when CTL_F34_INDIRECT2 =>
        if (memory_done_i = '1') then
          ctl_next_state <= CTL_F34_INDIRECT3;
        end if;
      when CTL_F34_INDIRECT3 =>
        ctl_next_state <= CTL_F34_DECODE2;
      when CTL_F34_DECODE2 =>
        case (insn_opcode(7 downto 2)) is
          when OPCODE_SHORT_LDCH | OPCODE_SHORT_TD | OPCODE_SHORT_RD |
               OPCODE_SHORT_WD =>
            if (insn_flag_n = '0' and insn_flag_i = '1') then
              ctl_next_state <= CTL_F34B_DECODE3;
            else
              ctl_next_state <= CTL_F34B_LOAD0;
            end if;
          when OPCODE_SHORT_STSW =>
            if (insn_flag_n = '0' and insn_flag_i = '1') then
              ctl_next_state <= CTL_ERROR;
            else
              ctl_next_state <= CTL_F34B_STSW;
            end if;
          when OPCODE_SHORT_STCH =>
            if (insn_flag_n = '0' and insn_flag_i = '1') then
              ctl_next_state <= CTL_ERROR;
            else
              ctl_next_state <= CTL_F34B_STCH;
            end if;
          when OPCODE_SHORT_STIL =>
            if (insn_flag_n = '0' and insn_flag_i = '1') then
              ctl_next_state <= CTL_ERROR;
            else
              ctl_next_state <= CTL_F34W_STIL;
            end if;
          when OPCODE_SHORT_STA | OPCODE_SHORT_STB | OPCODE_SHORT_STL |
               OPCODE_SHORT_STS | OPCODE_SHORT_STT | OPCODE_SHORT_STX =>
            if (insn_flag_n = '0' and insn_flag_i = '1') then
              ctl_next_state <= CTL_ERROR;
            else
              ctl_next_state <= CTL_F34W_STR0;
            end if;
          when OPCODE_SHORT_J | OPCODE_SHORT_JEQ | OPCODE_SHORT_JGT |
               OPCODE_SHORT_JLT =>
            ctl_next_state <= CTL_F34_JUMP;
          when OPCODE_SHORT_JSUB =>
            ctl_next_state <= CTL_F34_JSUB0;
          when others =>
            if (insn_flag_n = '0' and insn_flag_i = '1') then
              ctl_next_state <= CTL_F34W_DECODE3;
            else
              ctl_next_state <= CTL_F34W_LOAD0;
            end if;
        end case;

      -- format S34 instructions - device operations and load byte
      when CTL_F34B_LOAD0 =>
        if (memory_done_i = '1') then
          ctl_next_state <= CTL_F34B_LOAD1;
        end if;
      when CTL_F34B_LOAD1 =>
        ctl_next_state <= CTL_F34B_DECODE3;
      when CTL_F34B_DECODE3 =>
        case (insn_opcode(7 downto 2)) is
          when OPCODE_SHORT_LDCH =>
            ctl_next_state <= CTL_F34B_LDCH0;
          when OPCODE_SHORT_TD =>
            ctl_next_state <= CTL_F34B_TD;
          when OPCODE_SHORT_RD =>
            ctl_next_state <= CTL_F34B_RD0;
          when OPCODE_SHORT_WD =>
            ctl_next_state <= CTL_F34B_WD0;
          when others =>
        end case;
      when CTL_F34B_RD0 =>
        ctl_next_state <= CTL_F34B_RD1;
      when CTL_F34B_RD1 =>
        ctl_next_state <= CTL_F34B_RD2;
      when CTL_F34B_RD2 =>
        ctl_next_state <= CTL_F34B_RD3;
      when CTL_F34B_WD0 =>
        ctl_next_state <= CTL_F34B_WD1;
      when CTL_F34B_WD1 =>
        ctl_next_state <= CTL_F34B_WD2;
      when CTL_F34B_LDCH0 =>
        ctl_next_state <= CTL_F34B_LDCH1;

      -- format S34 instructions - store
      when CTL_F34B_STSW =>
        ctl_next_state <= CTL_F34_STORE2;
      when CTL_F34B_STCH =>
        ctl_next_state <= CTL_F34_STORE2;
      when CTL_F34W_STR0 =>
        ctl_next_state <= CTL_F34W_STR1;
      when CTL_F34W_STR1 =>
        ctl_next_state <= CTL_F34_STORE0;
      when CTL_F34W_STIL =>
        ctl_next_state <= CTL_F34_STORE0;
      when CTL_F34_STORE0 =>
        if (memory_done_i = '1') then
          ctl_next_state <= CTL_F34_STORE1;
        end if;
      when CTL_F34_STORE1 =>
        if (memory_done_i = '1') then
          ctl_next_state <= CTL_F34_STORE2;
        end if;
      when CTL_F34_STORE2 =>
        if (memory_done_i = '1') then
          if (enable_i = '1') then
            if (reg_interrupt = '1' and interrupt_i = '1') then
              ctl_next_state <= CTL_INT0;
            else
              ctl_next_state <= CTL_INSN0;
            end if;
          else
            ctl_next_state <= CTL_DISABLED;
          end if;
        end if;

      -- format S34 instructions - jump
      when CTL_F34_JSUB0 =>
        ctl_next_state <= CTL_F34_JSUB1;

      -- format S34 instructions - load, ALU and others
      when CTL_F34W_LOAD0 =>
        if (memory_done_i = '1') then
          ctl_next_state <= CTL_F34W_LOAD1;
        end if;
      when CTL_F34W_LOAD1 =>
        if (memory_done_i = '1') then
          ctl_next_state <= CTL_F34W_LOAD2;
        end if;
      when CTL_F34W_LOAD2 =>
        if (memory_done_i = '1') then
          ctl_next_state <= CTL_F34W_LOAD3;
        end if;
      when CTL_F34W_LOAD3 =>
        ctl_next_state <= CTL_F34W_DECODE3;
      when CTL_F34W_DECODE3 =>
        case (insn_opcode(7 downto 2)) is
          when OPCODE_SHORT_LDA | OPCODE_SHORT_LDB | OPCODE_SHORT_LDL |
               OPCODE_SHORT_LDS | OPCODE_SHORT_LDT | OPCODE_SHORT_LDX =>
            ctl_next_state <= CTL_F34W_LDR0;
          when OPCODE_SHORT_COMP =>
            ctl_next_state <= CTL_F34W_COMP;
          when OPCODE_SHORT_TIX =>
            ctl_next_state <= CTL_F34W_TIX0;
          when OPCODE_SHORT_ADD | OPCODE_SHORT_SUB | OPCODE_SHORT_MUL |
               OPCODE_SHORT_AND | OPCODE_SHORT_OR | OPCODE_SHORT_XOR =>
            ctl_next_state <= CTL_F34W_ALU0;
          when others =>
        end case;
      when CTL_F34W_LDR0 =>
        ctl_next_state <= CTL_F34W_LDR1;
      when CTL_F34W_TIX0 =>
        ctl_next_state <= CTL_F34W_TIX1;
      when CTL_F34W_TIX1 =>
        ctl_next_state <= CTL_F34W_TIX2;
      when CTL_F34W_ALU0 =>
        ctl_next_state <= CTL_F34W_ALU1;

      -- interrupt cycle
      when CTL_INT0 =>
        ctl_next_state <= CTL_INT1;
      when CTL_INT1 =>
        ctl_next_state <= CTL_INT2;
      when CTL_INT2 =>
        if (memory_done_i = '1') then
          ctl_next_state <= CTL_INT3;
        end if;
      when CTL_INT3 =>
        if (memory_done_i = '1') then
          ctl_next_state <= CTL_INT4;
        end if;
      when CTL_INT4 =>
        if (memory_done_i = '1') then
          ctl_next_state <= CTL_INT5;
        end if;
      when CTL_INT5 =>
        if (enable_i = '1') then
          ctl_next_state <= CTL_INSN0;
        else
          ctl_next_state <= CTL_DISABLED;
        end if;

      -- shared logic for last state of many instructions
      when CTL_F1_RINT | CTL_F2_ALU1 | CTL_F2_COMP | CTL_F2_TIX2 | CTL_F34_RSUB |
           CTL_F34B_TD | CTL_F34B_RD3 | CTL_F34B_WD2 | CTL_F34B_LDCH1 |
           CTL_F34_JUMP | CTL_F34_JSUB1 | CTL_F34W_LDR1 | CTL_F34W_COMP |
           CTL_F34W_TIX2 | CTL_F34W_ALU1 =>
        if (enable_i = '1') then
          if (reg_interrupt = '1' and interrupt_i = '1') then
            ctl_next_state <= CTL_INT0;
          else
            ctl_next_state <= CTL_INSN0;
          end if;
        else
          ctl_next_state <= CTL_DISABLED;
        end if;

      when others =>
    end case;
  end process;

  ctl_output_proc : process(ctl_state, insn_opcode, memory_done_i, insn_flag_n,
                            insn_flag_i, insn_flag_x, insn_flag_b, insn_flag_p,
                            insn_flag_e)
  begin
    error_o <= '0';
    memory_read_o <= '0';
    memory_write_o <= '0';
    port_read_strobe_o <= '0';
    port_write_strobe_o <= '0';
    interrupt_acknowledge_o <= '0';
    alu_operation <= ALU_ZERO;
    reg_general_write <= '0';
    reg_general_select_write <= '0';
    select_general <= SELECT_GENERAL_A;
    reg_operand1_write <= '0';
    reg_operand2_write <= '0';
    reg_operand3_write <= '0';
    reg_result_write <= '0';
    reg_special_target_write <= '0';
    reg_special_pc_write <= '0';
    reg_special_il_write <= '0';
    reg_special_cc_clear <= '0';
    reg_special_cc_write_left <= '0';
    reg_special_cc_write_right <= '0';
    reg_special_cc_save <= '0';
    reg_special_cc_restore <= '0';
    reg_special_pc_write_cond_lt <= '0';
    reg_special_pc_write_cond_eq <= '0';
    reg_special_pc_write_cond_gt <= '0';
    interrupt_disable <= '0';
    interrupt_enable <= '0';
    interrupt_move <= '0';
    reg_memory_data_write_result <= '0';
    reg_memory_data_write_mem <= (others => '0');
    reg_device_data_write_result <= '0';
    reg_device_data_write_dev <= '0';
    reg_instruction_write <= (others => '0');
    select_op1 <= SELECT_OP1_ROP1;
    select_op2 <= SELECT_OP2_CONE;
    select_addr <= SELECT_ADDR_PC;
    select_mem <= SELECT_MEM_BYTE0;
    case (ctl_state) is
      -- special state
      when CTL_ERROR =>
        error_o <= '1';
      when CTL_DISABLED =>

      -- instruction read & decode
      when CTL_INSN0 =>
        select_op1 <= SELECT_OP1_PC;
        select_op2 <= SELECT_OP2_CONE;
        alu_operation <= ALU_ADD;
        select_addr <= SELECT_ADDR_PC;
        memory_read_o <= '1';
        if (memory_done_i = '1') then
          reg_instruction_write(3) <= '1';
          reg_special_pc_write <= '1';
          interrupt_move <= '1';
        end if;
      when CTL_DECODE0 =>

      -- format 1 instructions
      when CTL_F1_EINT =>
        interrupt_enable <= '1';
      when CTL_F1_DINT =>
        interrupt_disable <= '1';
      when CTL_F1_RINT =>
        select_op1 <= SELECT_OP1_IL;
        alu_operation <= ALU_PASS1;
        reg_special_pc_write <= '1';
        reg_special_cc_restore <= '1';

      -- format 2 instructions
      when CTL_F2_INSN1 =>
        select_op1 <= SELECT_OP1_PC;
        select_op2 <= SELECT_OP2_CONE;
        alu_operation <= ALU_ADD;
        select_addr <= SELECT_ADDR_PC;
        memory_read_o <= '1';
        if (memory_done_i = '1') then
          reg_instruction_write(2) <= '1';
          reg_special_pc_write <= '1';
        end if;
      when CTL_F2_DECODE1 =>
        reg_operand1_write <= '1';
        reg_operand2_write <= '1';
      when CTL_F2_ALU0 =>
        select_op1 <= SELECT_OP1_ROP1;
        select_op2 <= SELECT_OP2_ROP2;
        reg_result_write <= '1';
        reg_general_select_write <= '1';
        case (insn_opcode) is
          when OPCODE_LONG_CLEAR | OPCODE_LONG_NOT |
               OPCODE_LONG_SHIFTL | OPCODE_LONG_SHIFTR =>
            select_general <= SELECT_GENERAL_R1;
          when others =>
            select_general <= SELECT_GENERAL_R2;
        end case;
        case (insn_opcode) is
          when OPCODE_LONG_CLEAR =>
            alu_operation <= ALU_ZERO;
          when OPCODE_LONG_RMO =>
            alu_operation <= ALU_PASS1;
          when OPCODE_LONG_ADDR =>
            alu_operation <= ALU_ADD;
          when OPCODE_LONG_SUBR =>
            alu_operation <= ALU_SUB;
          when OPCODE_LONG_MULR =>
            alu_operation <= ALU_MUL;
          when OPCODE_LONG_SHIFTL =>
            alu_operation <= ALU_SHIFTL;
          when OPCODE_LONG_SHIFTR =>
            alu_operation <= ALU_SHIFTR;
          when OPCODE_LONG_ANDR =>
            alu_operation <= ALU_AND;
          when OPCODE_LONG_ORR =>
            alu_operation <= ALU_OR;
          when OPCODE_LONG_XORR =>
            alu_operation <= ALU_XOR;
          when OPCODE_LONG_NOT =>
            alu_operation <= ALU_NOT;
          when others =>
        end case;
      when CTL_F2_ALU1 =>
        reg_general_write <= '1';
      when CTL_F2_COMP =>
        select_op1 <= SELECT_OP1_ROP1;
        select_op2 <= SELECT_OP2_ROP2;
        reg_special_cc_write_left <= '1';
      when CTL_F2_TIX0 =>
        select_op1 <= SELECT_OP1_X;
        select_op2 <= SELECT_OP2_CONE;
        alu_operation <= ALU_ADD;
        reg_result_write <= '1';
        reg_general_select_write <= '1';
        select_general <= SELECT_GENERAL_X;
      when CTL_F2_TIX1 =>
        reg_general_write <= '1';
      when CTL_F2_TIX2 =>
        select_op1 <= SELECT_OP1_ROP1;
        select_op2 <= SELECT_OP2_X;
        reg_special_cc_write_right <= '1';

      -- format S34 instructions
      when CTL_F34_INSN1 =>
        select_op1 <= SELECT_OP1_PC;
        select_op2 <= SELECT_OP2_CONE;
        alu_operation <= ALU_ADD;
        select_addr <= SELECT_ADDR_PC;
        memory_read_o <= '1';
        if (memory_done_i = '1') then
          reg_instruction_write(2) <= '1';
          reg_special_pc_write <= '1';
        end if;
      when CTL_F34_INSN2 =>
        select_op1 <= SELECT_OP1_PC;
        select_op2 <= SELECT_OP2_CONE;
        alu_operation <= ALU_ADD;
        select_addr <= SELECT_ADDR_PC;
        memory_read_o <= '1';
        if (memory_done_i = '1') then
          reg_instruction_write(1) <= '1';
          reg_special_pc_write <= '1';
        end if;
      when CTL_F34_INSN3 =>
        select_op1 <= SELECT_OP1_PC;
        select_op2 <= SELECT_OP2_CONE;
        alu_operation <= ALU_ADD;
        select_addr <= SELECT_ADDR_PC;
        memory_read_o <= '1';
        if (memory_done_i = '1') then
          reg_instruction_write(0) <= '1';
          reg_special_pc_write <= '1';
        end if;
      when CTL_F34_DECODE1 =>
        if (insn_flag_n = '0' and insn_flag_i = '0') then
          select_op1 <= SELECT_OP1_SIC;
        else
          if (insn_flag_e = '1') then
            select_op1 <= SELECT_OP1_F4USGN;
          else
            if (insn_flag_p = '1') then
              select_op1 <= SELECT_OP1_F3SGN;
            else
              select_op1 <= SELECT_OP1_F3USGN;
            end if;
          end if;
        end if;
        if (insn_flag_b = '1' and insn_flag_p = '0') then
          select_op2 <= SELECT_OP2_B;
          alu_operation <= ALU_ADD;
        elsif (insn_flag_b = '0' and insn_flag_p = '1') then
          select_op2 <= SELECT_OP2_PC;
          alu_operation <= ALU_ADD;
        else
          alu_operation <= ALU_PASS1;
        end if;
        reg_special_target_write <= '1';
      when CTL_F34_INDEXED =>
        select_op1 <= SELECT_OP1_TARGET;
        select_op2 <= SELECT_OP2_X;
        alu_operation <= ALU_ADD;
        reg_special_target_write <= '1';
      when CTL_F34_INDIRECT0 =>
        select_op1 <= SELECT_OP1_TARGET;
        select_op2 <= SELECT_OP2_CONE;
        alu_operation <= ALU_ADD;
        select_addr <= SELECT_ADDR_TARGET;
        memory_read_o <= '1';
        if (memory_done_i = '1') then
          reg_memory_data_write_mem(2) <= '1';
          reg_special_target_write <= '1';
        end if;
      when CTL_F34_INDIRECT1 =>
        select_op1 <= SELECT_OP1_TARGET;
        select_op2 <= SELECT_OP2_CONE;
        alu_operation <= ALU_ADD;
        select_addr <= SELECT_ADDR_TARGET;
        memory_read_o <= '1';
        if (memory_done_i = '1') then
          reg_memory_data_write_mem(1) <= '1';
          reg_special_target_write <= '1';
        end if;
      when CTL_F34_INDIRECT2 =>
        select_addr <= SELECT_ADDR_TARGET;
        memory_read_o <= '1';
        if (memory_done_i = '1') then
          reg_memory_data_write_mem(0) <= '1';
        end if;
      when CTL_F34_INDIRECT3 =>
        select_op1 <= SELECT_OP1_MEM;
        alu_operation <= ALU_PASS1;
        reg_special_target_write <= '1';
      when CTL_F34_DECODE2 =>

      -- format S34 instructions - rsub
      when CTL_F34_RSUB =>
        select_op2 <= SELECT_OP2_L;
        alu_operation <= ALU_PASS2;
        reg_special_pc_write <= '1';

      -- format S34 instructions - device operations and load byte
      when CTL_F34B_LOAD0 =>
        select_addr <= SELECT_ADDR_TARGET;
        memory_read_o <= '1';
        if (memory_done_i = '1') then
          reg_memory_data_write_mem(0) <= '1';
        end if;
      when CTL_F34B_LOAD1 =>
        select_op1 <= SELECT_OP1_MEM_BYTE;
        alu_operation <= ALU_PASS1;
        reg_special_target_write <= '1';
      when CTL_F34B_DECODE3 =>
        select_general <= SELECT_GENERAL_A;
        reg_general_select_write <= '1';
      when CTL_F34B_TD =>
        reg_special_cc_clear <= '1';
      when CTL_F34B_RD0 =>
      when CTL_F34B_RD1 =>
        port_read_strobe_o <= '1';
        reg_device_data_write_dev <= '1';
      when CTL_F34B_RD2 =>
        select_op1 <= SELECT_OP1_DEV;
        alu_operation <= ALU_PASS1;
        reg_result_write <= '1';
      when CTL_F34B_RD3 =>
        reg_general_write <= '1';
      when CTL_F34B_WD0 =>
        select_op2 <= SELECT_OP2_A;
        alu_operation <= ALU_PASS2;
        reg_device_data_write_result <= '1';
      when CTL_F34B_WD1 =>
      when CTL_F34B_WD2 =>
        port_write_strobe_o <= '1';
      when CTL_F34B_LDCH0 =>
        select_op1 <= SELECT_OP1_TARGET;
        alu_operation <= ALU_PASS1;
        reg_result_write <= '1';
      when CTL_F34B_LDCH1 =>
        reg_general_write <= '1';

      -- format S34 instructions - store
      when CTL_F34B_STSW =>
        select_op2 <= SELECT_OP2_SW;
        alu_operation <= ALU_PASS2;
        reg_memory_data_write_result <= '1';
      when CTL_F34B_STCH =>
        select_op2 <= SELECT_OP2_A;
        alu_operation <= ALU_PASS2;
        reg_memory_data_write_result <= '1';
      when CTL_F34W_STR0 =>
        reg_operand3_write <= '1';
      when CTL_F34W_STR1 =>
        select_op2 <= SELECT_OP2_ROP3;
        alu_operation <= ALU_PASS2;
        reg_memory_data_write_result <= '1';
      when CTL_F34_STORE0 =>
        select_op1 <= SELECT_OP1_TARGET;
        select_op2 <= SELECT_OP2_CONE;
        alu_operation <= ALU_ADD;
        select_addr <= SELECT_ADDR_TARGET;
        select_mem <= SELECT_MEM_BYTE2;
        memory_write_o <= '1';
        if (memory_done_i = '1') then
          reg_special_target_write <= '1';
        end if;
      when CTL_F34_STORE1 =>
        select_op1 <= SELECT_OP1_TARGET;
        select_op2 <= SELECT_OP2_CONE;
        alu_operation <= ALU_ADD;
        select_addr <= SELECT_ADDR_TARGET;
        select_mem <= SELECT_MEM_BYTE1;
        memory_write_o <= '1';
        if (memory_done_i = '1') then
          reg_special_target_write <= '1';
        end if;
      when CTL_F34_STORE2 =>
        select_addr <= SELECT_ADDR_TARGET;
        select_mem <= SELECT_MEM_BYTE0;
        memory_write_o <= '1';

      -- format S34 instructions - jump
      when CTL_F34_JUMP =>
        select_op1 <= SELECT_OP1_TARGET;
        alu_operation <= ALU_PASS1;
        case (insn_opcode(7 downto 2)) is
          when OPCODE_SHORT_J =>
            reg_special_pc_write <= '1';
          when OPCODE_SHORT_JLT =>
            reg_special_pc_write_cond_lt <= '1';
          when OPCODE_SHORT_JEQ =>
            reg_special_pc_write_cond_eq <= '1';
          when OPCODE_SHORT_JGT =>
            reg_special_pc_write_cond_gt <= '1';
          when others =>
        end case;
      when CTL_F34_JSUB0 =>
        select_general <= SELECT_GENERAL_L;
        reg_general_select_write <= '1';
        select_op1 <= SELECT_OP1_PC;
        alu_operation <= ALU_PASS1;
        reg_result_write <= '1';
      when CTL_F34_JSUB1 =>
        reg_general_write <= '1';
        select_op1 <= SELECT_OP1_TARGET;
        alu_operation <= ALU_PASS1;
        reg_special_pc_write <= '1';

      -- format S34 instructions - load, ALU and others
      when CTL_F34W_LOAD0 =>
        select_op1 <= SELECT_OP1_TARGET;
        select_op2 <= SELECT_OP2_CONE;
        alu_operation <= ALU_ADD;
        select_addr <= SELECT_ADDR_TARGET;
        memory_read_o <= '1';
        if (memory_done_i = '1') then
          reg_memory_data_write_mem(2) <= '1';
          reg_special_target_write <= '1';
        end if;
      when CTL_F34W_LOAD1 =>
        select_op1 <= SELECT_OP1_TARGET;
        select_op2 <= SELECT_OP2_CONE;
        alu_operation <= ALU_ADD;
        select_addr <= SELECT_ADDR_TARGET;
        memory_read_o <= '1';
        if (memory_done_i = '1') then
          reg_memory_data_write_mem(1) <= '1';
          reg_special_target_write <= '1';
        end if;
      when CTL_F34W_LOAD2 =>
        select_addr <= SELECT_ADDR_TARGET;
        memory_read_o <= '1';
        if (memory_done_i = '1') then
          reg_memory_data_write_mem(0) <= '1';
        end if;
      when CTL_F34W_LOAD3 =>
        select_op1 <= SELECT_OP1_MEM;
        alu_operation <= ALU_PASS1;
        reg_special_target_write <= '1';
      when CTL_F34W_DECODE3 =>
      when CTL_F34W_LDR0 =>
        select_general <= SELECT_GENERAL_LOAD_INSN;
        reg_general_select_write <= '1';
        select_op1 <= SELECT_OP1_TARGET;
        alu_operation <= ALU_PASS1;
        reg_result_write <= '1';
      when CTL_F34W_LDR1 =>
        reg_general_write <= '1';
      when CTL_F34W_COMP =>
        select_op1 <= SELECT_OP1_TARGET;
        select_op2 <= SELECT_OP2_A;
        reg_special_cc_write_right <= '1';
      when CTL_F34W_TIX0 =>
        select_op1 <= SELECT_OP1_X;
        select_op2 <= SELECT_OP2_CONE;
        alu_operation <= ALU_ADD;
        reg_result_write <= '1';
        reg_general_select_write <= '1';
        select_general <= SELECT_GENERAL_X;
      when CTL_F34W_TIX1 =>
        reg_general_write <= '1';
      when CTL_F34W_TIX2 =>
        select_op1 <= SELECT_OP1_TARGET;
        select_op2 <= SELECT_OP2_X;
        reg_special_cc_write_right <= '1';
      when CTL_F34W_ALU0 =>
        select_general <= SELECT_GENERAL_A;
        reg_general_select_write <= '1';
        reg_result_write <= '1';
        select_op1 <= SELECT_OP1_TARGET;
        select_op2 <= SELECT_OP2_A;
        case (insn_opcode(7 downto 2)) is
          when OPCODE_SHORT_ADD =>
            alu_operation <= ALU_ADD;
          when OPCODE_SHORT_SUB =>
            alu_operation <= ALU_SUB;
          when OPCODE_SHORT_MUL =>
            alu_operation <= ALU_MUL;
          when OPCODE_SHORT_AND =>
            alu_operation <= ALU_AND;
          when OPCODE_SHORT_OR =>
            alu_operation <= ALU_OR;
          when OPCODE_SHORT_XOR =>
            alu_operation <= ALU_XOR;
          when others =>
        end case;
      when CTL_F34W_ALU1 =>
        reg_general_write <= '1';

      -- interrupt cycle
      when CTL_INT0 =>
        interrupt_acknowledge_o <= '1';
        interrupt_disable <= '1';
        select_op1 <= SELECT_OP1_PC;
        alu_operation <= ALU_PASS1;
        reg_special_il_write <= '1';
        reg_special_cc_save <= '1';
      when CTL_INT1 =>
        select_op2 <= SELECT_OP2_CIV;
        alu_operation <= ALU_PASS2;
        reg_special_target_write <= '1';
      when CTL_INT2 =>
        select_op1 <= SELECT_OP1_TARGET;
        select_op2 <= SELECT_OP2_CONE;
        alu_operation <= ALU_ADD;
        select_addr <= SELECT_ADDR_TARGET;
        memory_read_o <= '1';
        if (memory_done_i = '1') then
          reg_memory_data_write_mem(2) <= '1';
          reg_special_target_write <= '1';
        end if;
      when CTL_INT3 =>
        select_op1 <= SELECT_OP1_TARGET;
        select_op2 <= SELECT_OP2_CONE;
        alu_operation <= ALU_ADD;
        select_addr <= SELECT_ADDR_TARGET;
        memory_read_o <= '1';
        if (memory_done_i = '1') then
          reg_memory_data_write_mem(1) <= '1';
          reg_special_target_write <= '1';
        end if;
      when CTL_INT4 =>
        select_addr <= SELECT_ADDR_TARGET;
        memory_read_o <= '1';
        if (memory_done_i = '1') then
          reg_memory_data_write_mem(0) <= '1';
        end if;
      when CTL_INT5 =>
        select_op1 <= SELECT_OP1_MEM;
        alu_operation <= ALU_PASS1;
        reg_special_pc_write <= '1';

      when others =>
    end case;
  end process;
end behavioral;
