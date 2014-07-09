library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_unsigned.all;

entity memory_controller is
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
end memory_controller;

architecture behavioral of memory_controller is
  -- buffer registers
  type data_select_type is (NONE, WRITE, READ_DEV1, READ_DEV2);
  signal data_select : data_select_type;
  signal next_data_select : data_select_type;
  signal data_select_write : std_logic;

  signal address : std_logic_vector(19 downto 0);
  signal data : std_logic_vector(7 downto 0);
  signal buffer_write1 : std_logic;
  signal buffer_write2 : std_logic;

  -- delay counter
  signal delay_counter : std_logic_vector(1 downto 0);
  signal delay_counter_reset : std_logic;
  signal delay_counter_done : std_logic;

  -- FSM
  type state_type is (READY, READ1, WRITE1, READ2, WRITE2);
  signal state : state_type;
  signal next_state : state_type;
begin
  micron_address_valid_o <= '0';
  micron_clock_o <= '0';
  micron_upper_byte_o <= '1';
  micron_lower_byte_o <= '0';
  micron_cre_o <= '0';

  -- buffer registers
  data_select_proc : process(clock_i)
  begin
    if (rising_edge(clock_i)) then
      if (reset_i = '1') then
        data_select <= NONE;
      else
        if (data_select_write = '1') then
          data_select <= next_data_select;
        else
          data_select <= data_select;
        end if;
      end if;
    end if;
  end process;

  addr_data_proct : process(clock_i)
  begin
    if (rising_edge(clock_i)) then
      if (reset_i = '1') then
        address <= (others => '0');
        data <= (others => '0');
      else
        if (buffer_write1 = '1') then
          address <= dev1_address_i;
          data <= dev1_data_out_i;
        elsif (buffer_write2 = '1') then
          address <= dev2_address_i;
          data <= dev2_data_out_i;
        else
          address <= address;
          data <= data;
        end if;
      end if;
    end if;
  end process;

  -- data and address bus
  micron_address_o <= "000" & address;

  bus_proc : process(data_select, address, data, micron_data_bus)
  begin
    dev1_data_in_o <= (others => '0');
    dev2_data_in_o <= (others => '0');
    micron_data_bus <= (others => 'Z');
    case (data_select) is
      when NONE =>
      when WRITE =>
        micron_data_bus <= data;
      when READ_DEV1 =>
        dev1_data_in_o <= micron_data_bus;
      when READ_DEV2 =>
        dev2_data_in_o <= micron_data_bus;
      when others =>
    end case;
  end process;

  -- delay counter
  delay_counter_proc : process(clock_i)
  begin
    if (rising_edge(clock_i)) then
      if (reset_i = '1' or delay_counter_reset = '1') then
        delay_counter <= (others => '0');
      else
        if (delay_counter = "11") then
          delay_counter <= (others => '0');
        else
          delay_counter <= delay_counter + 1;
        end if;
      end if;
    end if;
  end process;

  delay_counter_done_proc : process(delay_counter)
  begin
    if (delay_counter = "11") then
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
        state <= READY;
      else
        state <= next_state;
      end if;
    end if;
  end process;

  state_proc : process(state, dev1_read_i, dev1_write_i, dev2_read_i,
                       dev2_write_i, delay_counter_done)
  begin
    next_state <= state;
    case (state) is
      when READY =>
        if (dev1_read_i = '1') then
          next_state <= READ1;
        elsif (dev1_write_i = '1') then
          next_state <= WRITE1;
        elsif (dev2_read_i = '1') then
          next_state <= READ2;
        elsif (dev2_write_i = '1') then
          next_state <= WRITE2;
        end if;
      when READ1 | WRITE1 | READ2 | WRITE2 =>
        if (delay_counter_done = '1') then
          next_state <= READY;
        end if;
      when others =>
    end case;
  end process;

  output_proc : process(state, dev1_read_i, dev1_write_i, dev2_read_i,
                        dev2_write_i, delay_counter_done)
  begin
    micron_chip_enable_o <= '1';
    micron_output_enable_o <= '1';
    micron_write_enable_o <= '1';
    dev1_done_o <= '0';
    dev2_done_o <= '0';
    next_data_select <= NONE;
    data_select_write <= '0';
    buffer_write1 <= '0';
    buffer_write2 <= '0';
    delay_counter_reset <= '0';
    case (state) is
      when READY =>
        data_select_write <= '1';
        delay_counter_reset <= '1';
        if (dev1_read_i = '1') then
          buffer_write1 <= '1';
          next_data_select <= READ_DEV1;
        elsif (dev1_write_i = '1') then
          buffer_write1 <= '1';
          next_data_select <= WRITE;
        elsif (dev2_read_i = '1') then
          buffer_write2 <= '1';
          next_data_select <= READ_DEV2;
        elsif (dev2_write_i = '1') then
          buffer_write2 <= '1';
          next_data_select <= WRITE;
        end if;
      when READ1 =>
        micron_chip_enable_o <= '0';
        micron_output_enable_o <= '0';
        if (delay_counter_done = '1') then
          dev1_done_o <= '1';
        end if;
      when WRITE1 =>
        micron_chip_enable_o <= '0';
        micron_write_enable_o <= '0';
        if (delay_counter_done = '1') then
          dev1_done_o <= '1';
        end if;
      when READ2 =>
        micron_chip_enable_o <= '0';
        micron_output_enable_o <= '0';
        if (delay_counter_done = '1') then
          dev2_done_o <= '1';
        end if;
      when WRITE2 =>
        micron_chip_enable_o <= '0';
        micron_write_enable_o <= '0';
        if (delay_counter_done = '1') then
          dev2_done_o <= '1';
        end if;
      when others =>
    end case;
  end process;

end behavioral;

