library ieee;
use ieee.std_logic_1164.all;

entity top_module is
  Port (
    clock_i : in std_logic;
    reset_i : in std_logic;
    run_toggle_i : in std_logic;

    -- Micron CellularRAM connections
    micron_chip_enable_o : out std_logic;
    micron_output_enable_o : out std_logic;
    micron_write_enable_o : out std_logic;
    micron_address_o : out std_logic_vector(23 downto 1);
    micron_data_bus : inout std_logic_vector(7 downto 0);

    micron_address_valid_o : out std_logic;
    micron_clock_o : out std_logic;
    micron_upper_byte_o : out std_logic;
    micron_lower_byte_o : out std_logic;
    micron_cre_o : out std_logic;

    -- Intel Flash connections
    intel_chip_enable_o : out std_logic;

    -- Serial interface
    serial_i : in std_logic;
    serial_o : out std_logic;

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
end top_module;

architecture behavioral of top_module is
  component memory_controller
    Port (
      clock_i : in std_logic;
      reset_i : in std_logic;

      -- device 1
      dev1_read_i : in std_logic;
      dev1_write_i : in std_logic;
      dev1_address_i : in std_logic_vector(19 downto 0);
      dev1_data_in_o : out std_logic_vector(7 downto 0);
      dev1_data_out_i : in std_logic_vector(7 downto 0);
      dev1_done_o : out std_logic;

      -- device 2
      dev2_read_i : in std_logic;
      dev2_write_i : in std_logic;
      dev2_address_i : in std_logic_vector(19 downto 0);
      dev2_data_in_o : out std_logic_vector(7 downto 0);
      dev2_data_out_i : in std_logic_vector(7 downto 0);
      dev2_done_o : out std_logic;

      -- Micron CellularRAM connections
      micron_chip_enable_o : out std_logic;
      micron_output_enable_o : out std_logic;
      micron_write_enable_o : out std_logic;
      micron_address_o : out std_logic_vector(23 downto 1);
      micron_data_bus : inout std_logic_vector(7 downto 0);

      micron_address_valid_o : out std_logic;
      micron_clock_o : out std_logic;
      micron_upper_byte_o : out std_logic;
      micron_lower_byte_o : out std_logic;
      micron_cre_o : out std_logic
    );
  end component;

  component computer_interface
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
  end component;

  component sicxe_core
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
  end component;

  component device_subsystem
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
  end component;

  component run_control
    Port (
      clock_i : in std_logic;
      reset_i : in std_logic;
      start_i : in std_logic;
      stop_i : in std_logic;
      toggle_i : in std_logic;
      enable_o : out std_logic
    );
  end component;

  component interrupt_control
    Port (
      clock_i : in std_logic;
      reset_i : in std_logic;
      enable_i : in std_logic;
      event_i : in std_logic;
      acknowledge_i : in std_logic;
      interrupt_o : out std_logic
    );
  end component;

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

  -- computer interface
  signal computer_mem_read : std_logic;
  signal computer_mem_write : std_logic;
  signal computer_mem_address : std_logic_vector(19 downto 0);
  signal computer_mem_data_in : std_logic_vector(7 downto 0);
  signal computer_mem_data_out : std_logic_vector(7 downto 0);
  signal computer_mem_done : std_logic;
  signal computer_control_signals : std_logic_vector(3 downto 0);

  -- SIC/XE cpu
  signal cpu_reset : std_logic;
  signal cpu_enable : std_logic;
  signal cpu_error : std_logic;
  signal cpu_mem_read : std_logic;
  signal cpu_mem_write : std_logic;
  signal cpu_mem_address : std_logic_vector(19 downto 0);
  signal cpu_mem_data_in : std_logic_vector(7 downto 0);
  signal cpu_mem_data_out : std_logic_vector(7 downto 0);
  signal cpu_mem_done : std_logic;
  signal cpu_port_id : std_logic_vector(7 downto 0);
  signal cpu_port_in : std_logic_vector(7 downto 0);
  signal cpu_port_out : std_logic_vector(7 downto 0);
  signal cpu_port_read_strobe : std_logic;
  signal cpu_port_write_strobe : std_logic;
  signal cpu_interrupt : std_logic;
  signal cpu_interrupt_acknowledge : std_logic;
  signal cpu_interrupt_enabled : std_logic;

  -- devices
  signal seg7_stop : std_logic;
  signal device_event : std_logic;

  -- run control
  signal run_toggle : std_logic;

  -- interrupt event
  signal event : std_logic;

begin
  intel_chip_enable_o <= '1';  -- disable chip
  seg7_stop <= not cpu_enable;
  cpu_reset <= reset_i or computer_control_signals(0);
  event <= device_event or computer_control_signals(3);

  memory_cmpt : memory_controller
    port map (
      clock_i => clock_i,
      reset_i => reset_i,
      -- device 1
      dev1_read_i => computer_mem_read,
      dev1_write_i => computer_mem_write,
      dev1_address_i => computer_mem_address,
      dev1_data_in_o => computer_mem_data_in,
      dev1_data_out_i => computer_mem_data_out,
      dev1_done_o => computer_mem_done,
      -- device 2
      dev2_read_i => cpu_mem_read,
      dev2_write_i => cpu_mem_write,
      dev2_address_i => cpu_mem_address,
      dev2_data_in_o => cpu_mem_data_in,
      dev2_data_out_i => cpu_mem_data_out,
      dev2_done_o => cpu_mem_done,
      -- micron ram
      micron_chip_enable_o => micron_chip_enable_o,
      micron_output_enable_o => micron_output_enable_o,
      micron_write_enable_o => micron_write_enable_o,
      micron_address_o => micron_address_o,
      micron_data_bus => micron_data_bus,
      micron_address_valid_o => micron_address_valid_o,
      micron_clock_o => micron_clock_o,
      micron_upper_byte_o => micron_upper_byte_o,
      micron_lower_byte_o => micron_lower_byte_o,
      micron_cre_o => micron_cre_o
    );

  computer_cmpt : computer_interface
    port map (
      clock_i => clock_i,
      reset_i => reset_i,
      serial_i => serial_i,
      serial_o => serial_o,
      control_signals_o => computer_control_signals,
      memory_read_o => computer_mem_read,
      memory_write_o => computer_mem_write,
      memory_address_o => computer_mem_address,
      memory_data_in_i => computer_mem_data_in,
      memory_data_out_o => computer_mem_data_out,
      memory_done_i => computer_mem_done
    );

  sicxe_cmpt : sicxe_core
    port map (
      clock_i => clock_i,
      reset_i => cpu_reset,
      enable_i => cpu_enable,
      error_o => cpu_error,
      memory_read_o => cpu_mem_read,
      memory_write_o => cpu_mem_write,
      memory_address_o => cpu_mem_address,
      memory_data_in_i => cpu_mem_data_in,
      memory_data_out_o => cpu_mem_data_out,
      memory_done_i => cpu_mem_done,
      port_id_o => cpu_port_id,
      port_in_i => cpu_port_in,
      port_out_o => cpu_port_out,
      port_read_strobe_o => cpu_port_read_strobe,
      port_write_strobe_o => cpu_port_write_strobe,
      interrupt_i => cpu_interrupt,
      interrupt_acknowledge_o => cpu_interrupt_acknowledge,
      interrupt_enabled_o => cpu_interrupt_enabled
    );

  device_cmpt : device_subsystem
    port map (
      clock_i => clock_i,
      reset_i => reset_i,
      port_id_i => cpu_port_id,
      port_in_o => cpu_port_in,
      port_out_i => cpu_port_out,
      port_read_strobe_i => cpu_port_read_strobe,
      port_write_strobe_i => cpu_port_write_strobe,
      error_i => cpu_error,
      stop_i => seg7_stop,
      event_o => device_event,
      ps2_kbd_clock_i => ps2_kbd_clock_i,
      ps2_kbd_data_i => ps2_kbd_data_i,
      vga_hsync_o => vga_hsync_o,
      vga_vsync_o => vga_vsync_o,
      vga_red_o => vga_red_o,
      vga_green_o => vga_green_o,
      vga_blue_o => vga_blue_o,
      seg7_cathode_o => seg7_cathode_o,
      seg7_anode_o => seg7_anode_o,
      leds_o => leds_o,
      switches_i => switches_i,
      buttons_i => buttons_i
    );

  run_ctrl_cmpt : run_control
    port map (
      clock_i => clock_i,
      reset_i => reset_i,
      start_i => computer_control_signals(1),
      stop_i => computer_control_signals(2),
      toggle_i => run_toggle,
      enable_o => cpu_enable
    );

  interrupt_ctrl_cmpt : interrupt_control
    port map (
      clock_i => clock_i,
      reset_i => reset_i,
      enable_i => cpu_interrupt_enabled,
      event_i => event,
      acknowledge_i => cpu_interrupt_acknowledge,
      interrupt_o => cpu_interrupt
    );

  toggle_debouncer_cmpt : debouncer
    port map (
      clock_i => clock_i,
      reset_i => reset_i,
      input_i => run_toggle_i,
      output_o => open,
      change_on_o => run_toggle,
      change_off_o => open
    );

end behavioral;

