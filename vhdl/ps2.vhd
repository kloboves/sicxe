library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_unsigned.all;

entity ps2 is
  Port (
    clock_i : in std_logic;
    reset_i : in std_logic;
    kbd_clock_i : in std_logic;
    kbd_data_i : in std_logic;
    data_o : out std_logic_vector(7 downto 0);
    ready_o : out std_logic
  );
end ps2;

architecture behavioral of ps2 is
  -- keyboard input sync
  signal kbd_clock_sync : std_logic;
  signal kbd_data_sync : std_logic;

  -- pulse
  signal kbd_clock_delay1 : std_logic;
  signal pulse : std_logic;

  -- shift register
  signal shift_register : std_logic_vector(8 downto 0);
  signal shift_register_shift : std_logic;

  -- data register
  signal data : std_logic_vector(7 downto 0);

  -- ready
  signal ready : std_logic;
  signal ready_delay1 : std_logic;

  -- parity
  signal parity : std_logic;

  -- FSM
  type state_type is (WAITING, START, RECV0, RECV1, RECV2, RECV3, RECV4, RECV5, RECV6,
                      RECV7, RECV8);
  signal state : state_type;
  signal next_state : state_type;

begin
  data_o <= data;
  ready_o <= ready_delay1;

  -- keyboard input sync
  input_sync_proc : process(clock_i)
  begin
    if (rising_edge(clock_i)) then
      if (reset_i = '1') then
        kbd_clock_sync <= '0';
        kbd_data_sync <= '0';
      else
        kbd_clock_sync <= kbd_clock_i;
        kbd_data_sync <= kbd_data_i;
      end if;
    end if;
  end process;

  -- pulse
  clock_delay1_proc : process(clock_i)
  begin
    if (rising_edge(clock_i)) then
      if (reset_i = '1') then
        kbd_clock_delay1 <= '0';
      else
        kbd_clock_delay1 <= kbd_clock_sync;
      end if;
    end if;
  end process;

  pulse_proc : process(kbd_clock_sync, kbd_clock_delay1)
  begin
    if (kbd_clock_delay1 = '1' and kbd_clock_sync = '0') then
      pulse <= '1';
    else
      pulse <= '0';
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
          shift_register <= kbd_data_sync & shift_register(8 downto 1);
        else
          shift_register <= shift_register;
        end if;
      end if;
    end if;
  end process;

  -- data register
  data_proc : process(clock_i)
  begin
    if (rising_edge(clock_i)) then
      if (reset_i = '1') then
        data <= (others => '0');
      else
        if (ready = '1') then
          data <= shift_register(7 downto 0);
        else
          data <= data;
        end if;
      end if;
    end if;
  end process;

  -- ready
  ready_proc : process(clock_i)
  begin
    if (rising_edge(clock_i)) then
      if (reset_i = '1') then
        ready_delay1 <= '0';
      else
        ready_delay1 <= ready;
      end if;
    end if;
  end process;

  -- pairty
  parity_proc : process(shift_register)
  begin
    parity <= shift_register(8) xor shift_register(7) xor shift_register(6) xor
              shift_register(5) xor shift_register(4) xor shift_register(3) xor
              shift_register(2) xor shift_register(1) xor shift_register(0);
  end process;

  -- FSM
  sync_proc : process(clock_i)
  begin
    if (rising_edge(clock_i)) then
      if (reset_i = '1') then
        state <= WAITING;
      else
        state <= next_state;
      end if;
    end if;
  end process;

  state_proc : process(state, pulse)
  begin
    next_state <= state;
    case (state) is
      when WAITING =>
        if (pulse = '1') then
          next_state <= START;
        end if;
      when START =>
        if (pulse = '1') then
          next_state <= RECV0;
        end if;
      when RECV0 =>
        if (pulse = '1') then
          next_state <= RECV1;
        end if;
      when RECV1 =>
        if (pulse = '1') then
          next_state <= RECV2;
        end if;
      when RECV2 =>
        if (pulse = '1') then
          next_state <= RECV3;
        end if;
      when RECV3 =>
        if (pulse = '1') then
          next_state <= RECV4;
        end if;
      when RECV4 =>
        if (pulse = '1') then
          next_state <= RECV5;
        end if;
      when RECV5 =>
        if (pulse = '1') then
          next_state <= RECV6;
        end if;
      when RECV6 =>
        if (pulse = '1') then
          next_state <= RECV7;
        end if;
      when RECV7 =>
        if (pulse = '1') then
          next_state <= RECV8;
        end if;
      when RECV8 =>
        if (pulse = '1') then
          next_state <= WAITING;
        end if;
      when others =>
    end case;
  end process;

  output_proc : process(state, pulse, parity)
  begin
    shift_register_shift <= '0';
    ready <= '0';
    case (state) is
      when WAITING =>
      when START | RECV0 | RECV1 | RECV2 | RECV3 | RECV4 | RECV5 | RECV6 | RECV7 =>
        if (pulse = '1') then
          shift_register_shift <= '1';
        end if;
      when RECV8 =>
        if (pulse = '1' and parity = '1') then
          ready <= '1';
        end if;
      when others =>
    end case;
  end process;

end behavioral;
