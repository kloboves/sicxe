library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_unsigned.all;

entity computer_interface is
  Port (
    clock_i : in std_logic;
    reset_i : in std_logic;
    serial_i : in std_logic;
    serial_o : out std_logic;
    control_signals_o : out std_logic_vector(3 downto 0);
    memory_read_o : out std_logic;
    memory_write_o : out std_logic;
    memory_address_o : out std_logic_vector(19 downto 0);
    memory_data_in_i : in std_logic_vector(7 downto 0);
    memory_data_out_o : out std_logic_vector(7 downto 0);
    memory_done_i : in std_logic
  );
end computer_interface;

architecture behavioral of computer_interface is
  component rs232_in
    Port (
      clock_i : in std_logic;
      reset_i : in std_logic;
      input_i : in std_logic;
      data_o : out std_logic_vector(7 downto 0);
      receive_o : out std_logic
    );
  end component;

  component rs232_out
    Port (
      clock_i : in std_logic;
      reset_i : in std_logic;
      data_i : in std_logic_vector(7 downto 0);
      send_i : in std_logic;
      output_o : out std_logic;
      ready_o : out std_logic
    );
  end component;

  -- serial port signals
  signal in_data : std_logic_vector(7 downto 0);
  signal in_receive : std_logic;
  signal out_data : std_logic_vector(7 downto 0);
  signal out_ready : std_logic;
  signal out_send : std_logic;

  -- registers
  signal reg_command : std_logic_vector(7 downto 0);
  signal reg_command_write : std_logic;

  signal reg_address : std_logic_vector(19 downto 0);
  signal reg_address_write_byte2 : std_logic;
  signal reg_address_write_byte1 : std_logic;
  signal reg_address_write_byte0 : std_logic;
  signal reg_address_increment : std_logic;

  signal reg_data : std_logic_vector(7 downto 0);
  signal reg_data_write_in : std_logic;
  signal reg_data_write_mem : std_logic;

  signal reg_count : std_logic_vector(15 downto 0);
  signal reg_count_write_byte1 : std_logic;
  signal reg_count_write_byte0 : std_logic;
  signal reg_count_decrement : std_logic;

  signal reg_control_signals : std_logic_vector(3 downto 0);
  signal control_signals : std_logic_vector(3 downto 0);

  -- FSM
  type state_type is (LOCKED, PROTO_ERROR, KEY_GET1, KEY_GET2, KEY_GET3, KEY_GET4,
                      KEY_SEND1, KEY_SEND2, KEY_SEND3, UNLOCKED, COMMAND_ACCEPT,
                      GET_ADDR0, GET_ADDR1, GET_ADDR2, GET_COUNT0, GET_COUNT1,
                      WRITE_START, WRITE_IN, WRITE_MEM, READ_START, READ_MEM,
                      READ_OUT);
  signal state : state_type;
  signal next_state : state_type;

begin
  -- output connections
  memory_address_o <= reg_address;
  memory_data_out_o <= reg_data;
  control_signals_o <= reg_control_signals;

  -- rs232 components
  rs232_in_cmpt : rs232_in
    port map (
      clock_i => clock_i,
      reset_i => reset_i,
      input_i => serial_i,
      data_o => in_data,
      receive_o => in_receive
    );

  rs232_out_cmpt : rs232_out
    port map (
      clock_i => clock_i,
      reset_i => reset_i,
      data_i => out_data,
      send_i => out_send,
      output_o => serial_o,
      ready_o => out_ready
    );

  -- registers
  reg_command_proc : process(clock_i)
  begin
    if (rising_edge(clock_i)) then
      if (reset_i = '1') then
        reg_command <= (others => '0');
      else
        if (reg_command_write = '1') then
          reg_command <= in_data;
        else
          reg_command <= reg_command;
        end if;
      end if;
    end if;
  end process;

  reg_address_proc : process(clock_i)
  begin
    if (rising_edge(clock_i)) then
      if (reset_i = '1') then
        reg_address <= (others => '0');
      else
        reg_address <= reg_address;
        if (reg_address_write_byte2 = '1') then
          reg_address(19 downto 16) <= in_data(3 downto 0);
        elsif (reg_address_write_byte1 = '1') then
          reg_address(15 downto 8) <= in_data;
        elsif (reg_address_write_byte0 = '1') then
          reg_address(7 downto 0) <= in_data;
        elsif (reg_address_increment = '1') then
          reg_address <= reg_address + 1;
        end if;
      end if;
    end if;
  end process;

  reg_data_proc : process(clock_i)
  begin
    if (rising_edge(clock_i)) then
      if (reset_i = '1') then
        reg_data <= (others => '0');
      else
        if (reg_data_write_in = '1') then
          reg_data <= in_data;
        elsif (reg_data_write_mem = '1') then
          reg_data <= memory_data_in_i;
        else
          reg_data <= reg_data;
        end if;
      end if;
    end if;
  end process;

  reg_count_proc : process(clock_i)
  begin
    if (rising_edge(clock_i)) then
      if (reset_i = '1') then
        reg_count <= (others => '0');
      else
        reg_count <= reg_count;
        if (reg_count_write_byte1 = '1') then
          reg_count(15 downto 8) <= in_data;
        elsif (reg_count_write_byte0 = '1') then
          reg_count(7 downto 0) <= in_data;
        elsif (reg_count_decrement = '1') then
          reg_count <= reg_count - 1;
        end if;
      end if;
    end if;
  end process;

  control_signals_proc : process(clock_i)
  begin
    if (rising_edge(clock_i)) then
      if (reset_i = '1') then
        reg_control_signals <= (others => '0');
      else
        reg_control_signals <= control_signals;
      end if;
    end if;
  end process;

  -- FSM
  sync_proc : process(clock_i)
  begin
    if (rising_edge(clock_i)) then
      if (reset_i = '1') then
        state <= LOCKED;
      else
        state <= next_state;
      end if;
    end if;
  end process;

  state_proc : process(state, in_receive, in_data, out_ready, reg_command, reg_count,
                       memory_done_i)
  begin
    next_state <= state;
    case (state) is
      when PROTO_ERROR =>
        if (out_ready = '1') then
          next_state <= LOCKED;
        end if;
      when LOCKED =>
        if (in_receive = '1') then
          if (in_data = x"53") then
            next_state <= KEY_GET1;
          else
            next_state <= PROTO_ERROR;
          end if;
        end if;
      when KEY_GET1 =>
        if (in_receive = '1') then
          if (in_data = x"49") then
            next_state <= KEY_GET2;
          else
            next_state <= PROTO_ERROR;
          end if;
        end if;
      when KEY_GET2 =>
        if (in_receive = '1') then
          if (in_data = x"43") then
            next_state <= KEY_GET3;
          else
            next_state <= PROTO_ERROR;
          end if;
        end if;
      when KEY_GET3 =>
        if (in_receive = '1') then
          if (in_data = x"58") then
            next_state <= KEY_GET4;
          else
            next_state <= PROTO_ERROR;
          end if;
        end if;
      when KEY_GET4 =>
        if (in_receive = '1') then
          if (in_data = x"45") then
            next_state <= KEY_SEND1;
          else
            next_state <= PROTO_ERROR;
          end if;
        end if;
      when KEY_SEND1 =>
        if (out_ready = '1') then
          next_state <= KEY_SEND2;
        end if;
      when KEY_SEND2 =>
        if (out_ready = '1') then
          next_state <= KEY_SEND3;
        end if;
      when KEY_SEND3 =>
        if (out_ready = '1') then
          next_state <= UNLOCKED;
        end if;
      when UNLOCKED =>
        if (in_receive = '1') then
          if (in_data = x"00" or in_data = x"01" or in_data = x"02" or
              in_data = x"10" or in_data = x"11" or in_data = x"12" or
              in_data = x"13" or in_data = x"ff") then
            next_state <= COMMAND_ACCEPT;
          else
            next_state <= PROTO_ERROR;
          end if;
        end if;
      when COMMAND_ACCEPT =>
        if (out_ready = '1') then
          if (reg_command = x"ff") then  -- end session
            next_state <= LOCKED;
          elsif (reg_command = x"01" or reg_command = x"02") then
            next_state <= GET_ADDR0;
          else
            next_state <= UNLOCKED;
          end if;
        end if;
      when GET_ADDR0 =>
        if (in_receive = '1') then
          next_state <= GET_ADDR1;
        end if;
      when GET_ADDR1 =>
        if (in_receive = '1') then
          next_state <= GET_ADDR2;
        end if;
      when GET_ADDR2 =>
        if (in_receive = '1') then
          next_state <= GET_COUNT0;
        end if;
      when GET_COUNT0 =>
        if (in_receive = '1') then
          next_state <= GET_COUNT1;
        end if;
      when GET_COUNT1 =>
        if (in_receive = '1') then
          if (reg_command = x"02") then
            next_state <= WRITE_START;
          else
            next_state <= READ_START;
          end if;
        end if;
      when WRITE_START =>
        if (reg_count = x"0000") then
          next_state <= UNLOCKED;
        else
          next_state <= WRITE_IN;
        end if;
      when WRITE_IN =>
        if (in_receive = '1') then
          next_state <= WRITE_MEM;
        end if;
      when WRITE_MEM =>
        if (memory_done_i = '1') then
          next_state <= WRITE_START;
        end if;
      when READ_START =>
        if (reg_count = x"0000") then
          next_state <= UNLOCKED;
        else
          next_state <= READ_MEM;
        end if;
      when READ_MEM =>
        if (memory_done_i = '1') then
          next_state <= READ_OUT;
        end if;
      when READ_OUT =>
        if (out_ready = '1') then
          next_state <= READ_START;
        end if;
      when others =>
    end case;
  end process;

  output_proc : process(state, in_receive, in_data, out_ready, reg_command, reg_data,
                        memory_done_i)
  begin
    memory_read_o <= '0';
    memory_write_o <= '0';
    out_data <= (others => '0');
    out_send <= '0';
    reg_address_write_byte2 <= '0';
    reg_address_write_byte1 <= '0';
    reg_address_write_byte0 <= '0';
    reg_address_increment <= '0';
    reg_data_write_in <= '0';
    reg_data_write_mem <= '0';
    reg_count_write_byte1 <= '0';
    reg_count_write_byte0 <= '0';
    reg_count_decrement <= '0';
    reg_command_write <= '0';
    control_signals <= (others => '0');
    case (state) is
      when PROTO_ERROR =>
        if (out_ready = '1') then
          out_data <= x"58";
          out_send <= '1';
        end if;
      when LOCKED | KEY_GET1 | KEY_GET2 | KEY_GET3 | KEY_GET4 =>
      when KEY_SEND1 =>
        if (out_ready = '1') then
          out_data <= x"41";
          out_send <= '1';
        end if;
      when KEY_SEND2 =>
        if (out_ready = '1') then
          out_data <= x"43";
          out_send <= '1';
        end if;
      when KEY_SEND3 =>
        if (out_ready = '1') then
          out_data <= x"4b";
          out_send <= '1';
        end if;
      when UNLOCKED =>
        if (in_receive = '1'and
            (in_data = x"00" or in_data = x"01" or in_data = x"02" or
             in_data = x"10" or in_data = x"11" or in_data = x"12" or
             in_data = x"13" or in_data = x"ff")) then
          reg_command_write <= '1';
        end if;
      when COMMAND_ACCEPT =>
        if (out_ready = '1') then
          out_data <= x"4B";
          out_send <= '1';
          if (reg_command = x"10") then
            control_signals(0) <= '1';
          elsif (reg_command = x"11") then
            control_signals(1) <= '1';
          elsif (reg_command = x"12") then
            control_signals(2) <= '1';
          elsif (reg_command = x"13") then
            control_signals(3) <= '1';
          end if;
        end if;
      when GET_ADDR0 =>
        if (in_receive = '1') then
          reg_address_write_byte0 <= '1';
        end if;
      when GET_ADDR1 =>
        if (in_receive = '1') then
          reg_address_write_byte1 <= '1';
        end if;
      when GET_ADDR2 =>
        if (in_receive = '1') then
          reg_address_write_byte2 <= '1';
        end if;
      when GET_COUNT0 =>
        if (in_receive = '1') then
          reg_count_write_byte0 <= '1';
        end if;
      when GET_COUNT1 =>
        if (in_receive = '1') then
          reg_count_write_byte1 <= '1';
        end if;
      when WRITE_START =>
      when WRITE_IN =>
        if (in_receive = '1') then
          reg_data_write_in <= '1';
        end if;
      when WRITE_MEM =>
        memory_write_o <= '1';
        if (memory_done_i = '1') then
          reg_count_decrement <= '1';
          reg_address_increment <= '1';
        end if;
      when READ_START =>
      when READ_MEM =>
        memory_read_o <= '1';
        if (memory_done_i = '1') then
          reg_data_write_mem <= '1';
        end if;
      when READ_OUT =>
        if (out_ready = '1') then
          out_data <= reg_data;
          out_send <= '1';
          reg_count_decrement <= '1';
          reg_address_increment <= '1';
        end if;
      when others =>
    end case;
  end process;

end behavioral;

