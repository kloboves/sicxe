library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_unsigned.all;

entity debouncer is
  Port (
    clock_i : in std_logic;
    reset_i : in std_logic;
    input_i : in std_logic;
    output_o : out std_logic;
    change_on_o : out std_logic;
    change_off_o : out std_logic
  );
end debouncer;

architecture behavioral of debouncer is
  -- input sync
  signal input_sync : std_logic;

  -- delay counter
  signal delay_counter : std_logic_vector(15 downto 0);
  signal delay_counter_reset : std_logic;
  signal delay_counter_done : std_logic;

  -- FSM
  type state_type is (OFF_READY, OFF_WAIT, ON_READY, ON_WAIT);
  signal state : state_type;
  signal next_state : state_type;

begin
  -- input sync
  input_sync_proc : process(clock_i)
  begin
    if (rising_edge(clock_i)) then
      input_sync <= input_i;
    end if;
  end process;

  -- delay counter
  delay_counter_proc : process(clock_i)
  begin
    if (rising_edge(clock_i)) then
      if (reset_i = '1' or delay_counter_reset = '1') then
        delay_counter <= (others => '0');
      else
        if (delay_counter = "1100001101010000") then
          delay_counter <= (others => '0');
        else
          delay_counter <= delay_counter + 1;
        end if;
      end if;
    end if;
  end process;

  delay_counter_done_proc : process(delay_counter)
  begin
    if (delay_counter = "1100001101010000") then
      delay_counter_done <= '1';
    else
      delay_counter_done <= '0';
    end if;
  end process;

  -- FSM
  sync_proc : process(clock_i)
  begin
    if (rising_edge(clock_i)) then
      if (reset_i = '1') then
        if (input_sync = '1') then
          state <= ON_READY;
        else
          state <= OFF_READY;
        end if;
      else
        state <= next_state;
      end if;
    end if;
  end process;

  state_proc : process(state, input_sync, delay_counter_done)
  begin
    next_state <= state;
    case (state) is
      when OFF_READY =>
        if (input_sync = '1') then
          next_state <= OFF_WAIT;
        end if;
      when OFF_WAIT =>
        if (delay_counter_done = '1') then
          if (input_sync = '1') then
            next_state <= ON_READY;
          else
            next_state <= OFF_READY;
          end if;
        end if;
      when ON_READY =>
        if (input_sync = '0') then
          next_state <= ON_WAIT;
        end if;
      when ON_WAIT =>
        if (delay_counter_done = '1') then
          if (input_sync = '0') then
            next_state <= OFF_READY;
          else
            next_state <= ON_READY;
          end if;
        end if;
      when others =>
    end case;
  end process;

  output_proc : process(state, input_sync, delay_counter_done)
  begin
    output_o <= '0';
    change_on_o <= '0';
    change_off_o <= '0';
    delay_counter_reset <= '0';
    case (state) is
      when OFF_READY =>
        output_o <= '0';
        if (input_sync = '1') then
          delay_counter_reset <= '1';
        end if;
      when OFF_WAIT =>
        output_o <= '0';
        if (delay_counter_done = '1' and input_sync = '1') then
          change_on_o <= '1';
        end if;
      when ON_READY =>
        output_o <= '1';
        if (input_sync = '0') then
          delay_counter_reset <= '1';
        end if;
      when ON_WAIT =>
        output_o <= '1';
        if (delay_counter_done = '1' and input_sync = '0') then
          change_off_o <= '1';
        end if;
      when others =>
    end case;
  end process;

end behavioral;

