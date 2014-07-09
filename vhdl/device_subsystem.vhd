library ieee;
use ieee.std_logic_1164.all;

entity device_subsystem is
  Port (
    clock_i : in std_logic;
    reset_i : in std_logic;
    -- device port
    port_id_i : in std_logic_vector(7 downto 0);
    port_in_o : out std_logic_vector(7 downto 0);
    port_out_i : in std_logic_vector(7 downto 0);
    port_read_strobe_i : in std_logic;
    port_write_strobe_i : in std_logic;
    -- special
    error_i : in std_logic;
    stop_i : in std_logic;
    event_o : out std_logic;
    -- PS2
    ps2_kbd_clock_i : in std_logic;
    ps2_kbd_data_i : in std_logic;
    -- VGA
    vga_hsync_o : out std_logic;
    vga_vsync_o : out std_logic;
    vga_red_o : out std_logic_vector(2 downto 0);
    vga_green_o : out std_logic_vector(2 downto 0);
    vga_blue_o : out std_logic_vector(1 downto 0);
    -- 7 segment display
    seg7_cathode_o : out std_logic_vector(7 downto 0);
    seg7_anode_o : out std_logic_vector(3 downto 0);
    -- other
    leds_o : out std_logic_vector(7 downto 0);
    switches_i : in std_logic_vector(7 downto 0);
    buttons_i : in std_logic_vector(1 downto 0)
  );
end device_subsystem;

architecture behavioral of device_subsystem is
  component ps2
    Port (
      clock_i : in std_logic;
      reset_i : in std_logic;
      kbd_clock_i : in std_logic;
      kbd_data_i : in std_logic;
      data_o : out std_logic_vector(7 downto 0);
      ready_o : out std_logic
    );
  end component;

  component vga
    Port (
      clock_i : in std_logic;
      reset_i : in std_logic;
      -- framebuffer access
      data_i : in std_logic_vector(7 downto 0);
      write_row_i : in std_logic;
      write_column_i : in std_logic;
      write_color_i : in std_logic;
      -- VGA output
      hsync_o : out std_logic;
      vsync_o : out std_logic;
      red_o : out std_logic_vector(2 downto 0);
      green_o : out std_logic_vector(2 downto 0);
      blue_o : out std_logic_vector(1 downto 0)
    );
  end component;

  component seg7
    Port (
      clock_i : in std_logic;
      reset_i : in std_logic;
      -- special display modes
      error_i : in std_logic;
      stop_i : in std_logic;
      -- register access
      data_i : in std_logic_vector(7 downto 0);
      write_mode_i : in std_logic;
      write_raw_i : std_logic_vector(3 downto 0);
      write_hex_i : std_logic_vector(1 downto 0);
      -- display output
      cathode_o : out std_logic_vector(7 downto 0);
      anode_o : out std_logic_vector(3 downto 0)
    );
  end component;

  component gio_device
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
  end component;

  -- input devices
  signal ps2_ready : std_logic;
  signal gio_event : std_logic;

  signal ps2_data : std_logic_vector(7 downto 0);
  signal gio_switches : std_logic_vector(7 downto 0);
  signal gio_buttons : std_logic_vector(7 downto 0);

  -- output devices
  signal vga_write_row : std_logic;
  signal vga_write_column : std_logic;
  signal vga_write_color : std_logic;
  signal seg7_write_mode : std_logic;
  signal seg7_write_raw : std_logic_vector(3 downto 0);
  signal seg7_write_hex : std_logic_vector(1 downto 0);
  signal gio_write_leds : std_logic;

begin
  event_o <= ps2_ready or gio_event;

  input_proc : process(port_id_i, gio_switches, gio_buttons, ps2_data)
  begin
    port_in_o <= (others => '0');
    case (port_id_i) is
      when x"02" =>
        port_in_o <= gio_switches;
      when x"03" =>
        port_in_o <= gio_buttons;
      when x"04" =>
        port_in_o <= ps2_data;
      when others =>
    end case;
  end process;

  output_proc : process(port_id_i, port_write_strobe_i)
  begin
    vga_write_row <= '0';
    vga_write_column <= '0';
    vga_write_color <= '0';
    seg7_write_mode <= '0';
    seg7_write_raw <= (others => '0');
    seg7_write_hex <= (others => '0');
    gio_write_leds <= '0';
    case (port_id_i) is
      when x"05" =>
        gio_write_leds <= port_write_strobe_i;
      when x"06" =>
        seg7_write_mode <= port_write_strobe_i;
      when x"07" =>
        seg7_write_hex(0) <= port_write_strobe_i;
      when x"08" =>
        seg7_write_hex(1) <= port_write_strobe_i;
      when x"09" =>
        seg7_write_raw(0) <= port_write_strobe_i;
      when x"0a" =>
        seg7_write_raw(1) <= port_write_strobe_i;
      when x"0b" =>
        seg7_write_raw(2) <= port_write_strobe_i;
      when x"0c" =>
        seg7_write_raw(3) <= port_write_strobe_i;
      when x"0d" =>
        vga_write_row <= port_write_strobe_i;
      when x"0e" =>
        vga_write_column <= port_write_strobe_i;
      when x"0f" =>
        vga_write_color <= port_write_strobe_i;
      when others =>
    end case;
  end process;

  ps2_cmpt : ps2
    port map (
      clock_i => clock_i,
      reset_i => reset_i,
      kbd_clock_i => ps2_kbd_clock_i,
      kbd_data_i => ps2_kbd_data_i,
      data_o => ps2_data,
      ready_o => ps2_ready
    );

  vga_cmpt : vga
    port map (
      clock_i => clock_i,
      reset_i => reset_i,
      data_i => port_out_i,
      write_row_i => vga_write_row,
      write_column_i => vga_write_column,
      write_color_i => vga_write_color,
      hsync_o => vga_hsync_o,
      vsync_o => vga_vsync_o,
      red_o => vga_red_o,
      green_o => vga_green_o,
      blue_o => vga_blue_o
    );

  seg7_cmpt : seg7
    port map (
      clock_i => clock_i,
      reset_i => reset_i,
      error_i => error_i,
      stop_i => stop_i,
      data_i => port_out_i,
      write_mode_i => seg7_write_mode,
      write_raw_i => seg7_write_raw,
      write_hex_i => seg7_write_hex,
      cathode_o => seg7_cathode_o,
      anode_o => seg7_anode_o
    );

  gio_cmpt : gio_device
    port map (
      clock_i => clock_i,
      reset_i => reset_i,
      switches_data_o => gio_switches,
      buttons_data_o => gio_buttons,
      event_o => gio_event,
      data_i => port_out_i,
      write_leds_i => gio_write_leds,
      leds_o => leds_o,
      switches_i => switches_i,
      buttons_i => buttons_i
    );

end behavioral;
