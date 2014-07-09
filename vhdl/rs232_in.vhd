library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_unsigned.all;

entity rs232_in is
  Port (
    clock_i : in std_logic;
    reset_i : in std_logic;
    input_i : in std_logic;
    data_o : out std_logic_vector(7 downto 0);
    receive_o : out std_logic
  );
end rs232_in;

architecture behavioral of rs232_in is
  -- input sync
  signal input_sync : std_logic;

  -- delay counter
  signal delay_counter : std_logic_vector(8 downto 0);
  signal delay_counter_reset : std_logic;
  signal delay_counter_half : std_logic;

  -- shift register
  signal shift_register : std_logic_vector(7 downto 0);
  signal shift_register_shift : std_logic;

  -- FSM
  type state_type is (READY, WAIT_HALF, RECV_START, RECV0, RECV1, RECV2, RECV3,
                      RECV4, RECV5, RECV6, RECV7, RECV_END);
  signal state : state_type;
  signal next_state : state_type;

begin
  data_o <= shift_register;

  -- input sync
  input_sync_proc : process(clock_i)
  begin
    if (rising_edge(clock_i)) then
      input_sync <= input_i;
    end if;
  end process;

  -- delay counter (set for 115200 boud)
  delay_counter_proc : process(clock_i)
  begin
    if (rising_edge(clock_i)) then
      if (reset_i = '1' or delay_counter_reset = '1') then
        delay_counter <= (others => '0');
      else
        if (delay_counter = "110110010") then
          delay_counter <= (others => '0');
        else
          delay_counter <= delay_counter + 1;
        end if;
      end if;
    end if;
  end process;

  delay_counter_half_proc : process(delay_counter)
  begin
    if (delay_counter = "011011001") then
      delay_counter_half <= '1';
    else
      delay_counter_half <= '0';
    end if;
  end process;

  -- shift register
  shift_register_proc : process(clock_i)
  begin
    if (rising_edge(clock_i)) then
      if (reset_i = '1') then
        shift_register <= (others => '0');
      else
        if (shift_register_shift = '1') then
          shift_register <= input_sync & shift_register(7 downto 1);
        else
          shift_register <= shift_register;
        end if;
      end if;
    end if;
  end process;

  -- FSM
  sync_proc : process(clock_i)
  begin
    if (rising_edge(clock_i)) then
      if (reset_i = '1') then
        state <= READY;
      else
        state <= next_state;
      end if;
    end if;
  end process;

  state_proc : process(state, input_sync, delay_counter_half)
  begin
    next_state <= state;
    case (state) is
      when READY =>
        if (input_sync = '0') then
          next_state <= WAIT_HALF;
        end if;
      when WAIT_HALF =>
        if (delay_counter_half = '1') then
          next_state <= RECV_START;
        end if;
      when RECV_START =>
        if (delay_counter_half = '1') then
          next_state <= RECV0;
        end if;
      when RECV0 =>
        if (delay_counter_half = '1') then
          next_state <= RECV1;
        end if;
      when RECV1 =>
        if (delay_counter_half = '1') then
          next_state <= RECV2;
        end if;
      when RECV2 =>
        if (delay_counter_half = '1') then
          next_state <= RECV3;
        end if;
      when RECV3 =>
        if (delay_counter_half = '1') then
          next_state <= RECV4;
        end if;
      when RECV4 =>
        if (delay_counter_half = '1') then
          next_state <= RECV5;
        end if;
      when RECV5 =>
        if (delay_counter_half = '1') then
          next_state <= RECV6;
        end if;
      when RECV6 =>
        if (delay_counter_half = '1') then
          next_state <= RECV7;
        end if;
      when RECV7 =>
        if (delay_counter_half = '1') then
          next_state <= RECV_END;
        end if;
      when RECV_END =>
        if (input_sync = '1') then
          next_state <= READY;
        end if;
      when others =>
    end case;
  end process;

  output_proc : process(state, input_sync, delay_counter_half)
  begin
    delay_counter_reset <= '0';
    shift_register_shift <= '0';
    receive_o <= '0';
    case (state) is
      when READY =>
        if (input_sync = '0') then
          delay_counter_reset <= '1';
        end if;
      when WAIT_HALF =>
      when RECV_START | RECV0 | RECV1 | RECV2 | RECV3 | RECV4 | RECV5 | RECV6 =>
        if (delay_counter_half = '1') then
          shift_register_shift <= '1';
        end if;
      when RECV7 =>
        if (delay_counter_half = '1') then
          receive_o <= '1';
        end if;
      when RECV_END =>
      when others =>
    end case;
  end process;

end behavioral;

