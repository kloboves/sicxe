library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_unsigned.all;

-- Data bit connections on display:
--
--    --0--
--   |     |
--   1     2
--   |     |
--    --3--
--   |     |
--   4     5
--   |     |
--    --6--  [7] (dot)

entity seg7 is
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
end seg7;

architecture behavioral of seg7 is
  -- registers
  signal reg_mode : std_logic_vector(1 downto 0);
  signal reg_raw : std_logic_vector(31 downto 0);
  signal reg_hex : std_logic_vector(15 downto 0);

  -- delay counter
  signal delay_counter : std_logic_vector(14 downto 0);
  signal delay_counter_done : std_logic;

  -- current digit
  signal digit_counter : std_logic_vector(1 downto 0);
  signal digit_raw : std_logic_vector(7 downto 0);
  signal digit_hex : std_logic_vector(3 downto 0);

  -- output
  signal cathode : std_logic_vector(7 downto 0);
  signal anode : std_logic_vector(3 downto 0);
  signal cathode_delay1 : std_logic_vector(7 downto 0);
  signal anode_delay1 : std_logic_vector(3 downto 0);

begin
  -- registers
  reg_proc : process(clock_i)
  begin
    if (rising_edge(clock_i)) then
      if (reset_i = '1') then
        reg_mode <= (others => '0');
        reg_raw <= (others => '0');
        reg_hex <= (others => '0');
      else
        reg_mode <= reg_mode;
        reg_raw <= reg_raw;
        reg_hex <= reg_hex;
        if (write_mode_i = '1') then
          reg_mode <= data_i(1 downto 0);
        end if;
        if (write_raw_i(3) = '1') then
          reg_raw(31 downto 24) <= data_i;
        end if;
        if (write_raw_i(2) = '1') then
          reg_raw(23 downto 16) <= data_i;
        end if;
        if (write_raw_i(1) = '1') then
          reg_raw(15 downto 8) <= data_i;
        end if;
        if (write_raw_i(0) = '1') then
          reg_raw(7 downto 0) <= data_i;
        end if;
        if (write_hex_i(1) = '1') then
          reg_hex(15 downto 8) <= data_i;
        end if;
        if (write_hex_i(0) = '1') then
          reg_hex(7 downto 0) <= data_i;
        end if;
      end if;
    end if;
  end process;

  -- delay counter
  delay_counter_proc : process(clock_i)
  begin
    if (rising_edge(clock_i)) then
      if (reset_i = '1') then
        delay_counter <= (others => '0');
      else
        if (delay_counter = "111111111111111") then
          delay_counter <= (others => '0');
        else
          delay_counter <= delay_counter + 1;
        end if;
      end if;
    end if;
  end process;

  delay_counter_done_proc : process(delay_counter)
  begin
    if (delay_counter = "111111111111111") then
      delay_counter_done <= '1';
    else
      delay_counter_done <= '0';
    end if;
  end process;

  -- current digit
  digit_counter_proc : process(clock_i)
  begin
    if (rising_edge(clock_i)) then
      if (reset_i = '1') then
        digit_counter <= (others => '0');
      else
        if (delay_counter_done = '1') then
          digit_counter <= digit_counter + 1;
        else
          digit_counter <= digit_counter;
        end if;
      end if;
    end if;
  end process;

  digit_proc : process(digit_counter, reg_raw, reg_hex)
  begin
    case (digit_counter) is
      when "00" =>
        digit_raw <= reg_raw(7 downto 0);
        digit_hex <= reg_hex(3 downto 0);
      when "01" =>
        digit_raw <= reg_raw(15 downto 8);
        digit_hex <= reg_hex(7 downto 4);
      when "10" =>
        digit_raw <= reg_raw(23 downto 16);
        digit_hex <= reg_hex(11 downto 8);
      when "11" =>
        digit_raw <= reg_raw(31 downto 24);
        digit_hex <= reg_hex(15 downto 12);
      when others =>
        digit_raw <= (others => '0');
        digit_hex <= (others => '0');
    end case;
  end process;

  -- output
  cathode_o <= cathode_delay1;
  anode_o <= anode_delay1;

  delay_proc : process(clock_i)
  begin
    if (rising_edge(clock_i)) then
      if (reset_i = '1') then
        cathode_delay1 <= (others => '0');
        anode_delay1 <= (others => '1');
      else
        cathode_delay1 <= cathode;
        anode_delay1 <= anode;
      end if;
    end if;
  end process;

  anode_proc : process(digit_counter)
  begin
    case (digit_counter) is
      when "00" => anode <= "1110";
      when "01" => anode <= "1101";
      when "10" => anode <= "1011";
      when "11" => anode <= "0111";
      when others => anode <= (others => '1');
    end case;
  end process;

  cathode_proc : process(error_i, stop_i, digit_counter, reg_mode, digit_raw, digit_hex)
  begin
    if (error_i = '1') then
      case (digit_counter) is
        when "00" => cathode <= "11111111"; -- blank
        when "01" => cathode <= "11100111"; -- r
        when "10" => cathode <= "11100111"; -- r
        when "11" => cathode <= "10100100"; -- E
        when others => cathode <= (others => '1');
      end case;
    elsif (stop_i = '1') then
      case (digit_counter) is
        when "00" => cathode <= "11100000"; -- P
        when "01" => cathode <= "10001000"; -- O
        when "10" => cathode <= "10100101"; -- t
        when "11" => cathode <= "10010100"; -- S
        when others => cathode <= (others => '1');
      end case;
    else
      if (reg_mode = "00") then
        cathode <= not digit_raw;
      else
        if ((reg_mode = "01" and (digit_counter = "10" or digit_counter = "11")) or
            (reg_mode = "10" and (digit_counter = "00" or digit_counter = "01"))) then
          cathode <= not digit_raw;
        else
          case digit_hex is
            when "0000" => cathode <= "10001000";
            when "0001" => cathode <= "11011011";
            when "0010" => cathode <= "10100010";
            when "0011" => cathode <= "10010010";
            when "0100" => cathode <= "11010001";
            when "0101" => cathode <= "10010100";
            when "0110" => cathode <= "10000100";
            when "0111" => cathode <= "11011010";
            when "1000" => cathode <= "10000000";
            when "1001" => cathode <= "10010000";
            when "1010" => cathode <= "11000000";
            when "1011" => cathode <= "10000101";
            when "1100" => cathode <= "10101100";
            when "1101" => cathode <= "10000011";
            when "1110" => cathode <= "10100100";
            when "1111" => cathode <= "11100100";
            when others => cathode <= (others => '1');
          end case;
        end if;
      end if;
    end if;
  end process;

end behavioral;
