library ieee;
use ieee.std_logic_1164.all;

entity gio_device is
  Port (
    clock_i : in std_logic;
    reset_i : in std_logic;
    -- device access
    switches_data_o : out std_logic_vector(7 downto 0);
    buttons_data_o : out std_logic_vector(7 downto 0);
    event_o : out std_logic;
    data_i : in std_logic_vector(7 downto 0);
    write_leds_i : std_logic;
    -- physical connections
    leds_o : out std_logic_vector(7 downto 0);
    switches_i : in std_logic_vector(7 downto 0);
    buttons_i : in std_logic_vector(1 downto 0)
  );
end gio_device;

architecture behavioral of gio_device is
  component debouncer
    Port (
      clock_i : in std_logic;
      reset_i : in std_logic;
      input_i : in std_logic;
      output_o : out std_logic;
      change_on_o : out std_logic;
      change_off_o : out std_logic
    );
  end component;

  -- leds
  signal reg_leds : std_logic_vector(7 downto 0);

  -- switches and buttons
  signal switches : std_logic_vector(7 downto 0);
  signal buttons : std_logic_vector(1 downto 0);

  -- events
  signal switches_event1 : std_logic_vector(7 downto 0);
  signal switches_event2 : std_logic_vector(7 downto 0);
  signal buttons_event1 : std_logic_vector(1 downto 0);
  signal buttons_event2 : std_logic_vector(1 downto 0);

begin
  leds_proc : process(clock_i)
  begin
    if (rising_edge(clock_i)) then
      if (reset_i = '1') then
        reg_leds <= (others => '0');
      else
        if (write_leds_i = '1') then
          reg_leds <= data_i;
        else
          reg_leds <= reg_leds;
        end if;
      end if;
    end if;
  end process;

  leds_o <= reg_leds;
  switches_data_o <= switches;
  buttons_data_o <= "000000" & buttons;
  event_o <= switches_event1(0) or switches_event1(1) or
             switches_event1(2) or switches_event1(3) or
             switches_event1(4) or switches_event1(5) or
             switches_event1(6) or switches_event1(7) or
             switches_event2(0) or switches_event2(1) or
             switches_event2(2) or switches_event2(3) or
             switches_event2(4) or switches_event2(5) or
             switches_event2(6) or switches_event2(7) or
             buttons_event1(0) or buttons_event1(1) or
             buttons_event2(0) or buttons_event2(1);

  sw0_debounce_cmpt : debouncer
    port map (
      clock_i => clock_i,
      reset_i => reset_i,
      input_i => switches_i(0),
      output_o => switches(0),
      change_on_o => switches_event1(0),
      change_off_o => switches_event2(0)
    );

  sw1_debounce_cmpt : debouncer
    port map (
      clock_i => clock_i,
      reset_i => reset_i,
      input_i => switches_i(1),
      output_o => switches(1),
      change_on_o => switches_event1(1),
      change_off_o => switches_event2(1)
    );

  sw2_debounce_cmpt : debouncer
    port map (
      clock_i => clock_i,
      reset_i => reset_i,
      input_i => switches_i(2),
      output_o => switches(2),
      change_on_o => switches_event1(2),
      change_off_o => switches_event2(2)
    );

  sw3_debounce_cmpt : debouncer
    port map (
      clock_i => clock_i,
      reset_i => reset_i,
      input_i => switches_i(3),
      output_o => switches(3),
      change_on_o => switches_event1(3),
      change_off_o => switches_event2(3)
    );

  sw4_debounce_cmpt : debouncer
    port map (
      clock_i => clock_i,
      reset_i => reset_i,
      input_i => switches_i(4),
      output_o => switches(4),
      change_on_o => switches_event1(4),
      change_off_o => switches_event2(4)
    );

  sw5_debounce_cmpt : debouncer
    port map (
      clock_i => clock_i,
      reset_i => reset_i,
      input_i => switches_i(5),
      output_o => switches(5),
      change_on_o => switches_event1(5),
      change_off_o => switches_event2(5)
    );

  sw6_debounce_cmpt : debouncer
    port map (
      clock_i => clock_i,
      reset_i => reset_i,
      input_i => switches_i(6),
      output_o => switches(6),
      change_on_o => switches_event1(6),
      change_off_o => switches_event2(6)
    );

  sw7_debounce_cmpt : debouncer
    port map (
      clock_i => clock_i,
      reset_i => reset_i,
      input_i => switches_i(7),
      output_o => switches(7),
      change_on_o => switches_event1(7),
      change_off_o => switches_event2(7)
    );

  btn0_debounce_cmpt : debouncer
    port map (
      clock_i => clock_i,
      reset_i => reset_i,
      input_i => buttons_i(0),
      output_o => buttons(0),
      change_on_o => buttons_event1(0),
      change_off_o => buttons_event2(0)
    );

  btn1_debounce_cmpt : debouncer
    port map (
      clock_i => clock_i,
      reset_i => reset_i,
      input_i => buttons_i(1),
      output_o => buttons(1),
      change_on_o => buttons_event1(1),
      change_off_o => buttons_event2(1)
    );

end behavioral;
