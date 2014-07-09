library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_unsigned.all;

entity rs232_out is
  Port (
    clock_i : in std_logic;
    reset_i : in std_logic;
    data_i : in std_logic_vector(7 downto 0);
    send_i : in std_logic;
    output_o : out std_logic;
    ready_o : out std_logic
  );
end rs232_out;

architecture behavioral of rs232_out is
  -- delay counter
  signal delay_counter : std_logic_vector(8 downto 0);
  signal delay_counter_reset : std_logic;
  signal delay_counter_done : std_logic;

  -- shift register
  signal shift_register : std_logic_vector(7 downto 0);
  signal shift_register_save : std_logic;
  signal shift_register_shift : std_logic;

  -- FSM
  type state_type is (READY, SEND_START, SEND0, SEND1, SEND2, SEND3,
                      SEND4, SEND5, SEND6, SEND7, SEND_END);
  signal state : state_type;
  signal next_state : state_type;

begin
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

  delay_counter_done_proc : process(delay_counter)
  begin
    if (delay_counter = "110110010") then
      delay_counter_done <= '1';
    else
      delay_counter_done <= '0';
    end if;
  end process;

  -- shift register
  shift_register_proc : process(clock_i)
  begin
    if (rising_edge(clock_i)) then
      if (reset_i = '1') then
        shift_register <= (others => '0');
      else
        if (shift_register_save = '1') then
          shift_register <= data_i;
        else
          if (shift_register_shift = '1') then
            shift_register <= '0' & shift_register(7 downto 1);
          else
            shift_register <= shift_register;
          end if;
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

  state_proc : process(state, send_i, delay_counter_done)
  begin
    next_state <= state;
    case (state) is
      when READY =>
        if (send_i = '1') then
          next_state <= SEND_START;
        end if;
      when SEND_START =>
        if (delay_counter_done = '1') then
          next_state <= SEND0;
        end if;
      when SEND0 =>
        if (delay_counter_done = '1') then
          next_state <= SEND1;
        end if;
      when SEND1 =>
        if (delay_counter_done = '1') then
          next_state <= SEND2;
        end if;
      when SEND2 =>
        if (delay_counter_done = '1') then
          next_state <= SEND3;
        end if;
      when SEND3 =>
        if (delay_counter_done = '1') then
          next_state <= SEND4;
        end if;
      when SEND4 =>
        if (delay_counter_done = '1') then
          next_state <= SEND5;
        end if;
      when SEND5 =>
        if (delay_counter_done = '1') then
          next_state <= SEND6;
        end if;
      when SEND6 =>
        if (delay_counter_done = '1') then
          next_state <= SEND7;
        end if;
      when SEND7 =>
        if (delay_counter_done = '1') then
          next_state <= SEND_END;
        end if;
      when SEND_END =>
        if (delay_counter_done = '1') then
          next_state <= READY;
        end if;
      when others =>
    end case;
  end process;

  output_proc : process(state, send_i, delay_counter_done, shift_register)
  begin
    delay_counter_reset <= '0';
    shift_register_save <= '0';
    shift_register_shift <= '0';
    output_o <= '1';
    ready_o <= '0';
    case (state) is
      when READY =>
        ready_o <= '1';
        if (send_i = '1') then
          delay_counter_reset <= '1';
          shift_register_save <= '1';
        end if;
      when SEND_START =>
        output_o <= '0';
      when SEND0 | SEND1 | SEND2 | SEND3 | SEND4 | SEND5 | SEND6 =>
        output_o <= shift_register(0);
        if (delay_counter_done = '1') then
          shift_register_shift <= '1';
        end if;
      when SEND7 =>
        output_o <= shift_register(0);
      when SEND_END =>
      when others =>
    end case;
  end process;

end behavioral;

