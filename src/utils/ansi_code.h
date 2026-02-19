// Derived from RabaDabaDoba ansi color code
// => github.com/RabaDabaDoba

#pragma once

/*
 * This is free and unencumbered software released into the public domain.
 *
 * For more information, please refer to <https://unlicense.org>
 */

namespace ansi_code {

// Regular text
constexpr const char *BLK = "\e[0;30m";
constexpr const char *RED = "\e[0;31m";
constexpr const char *GRN = "\e[0;32m";
constexpr const char *YEL = "\e[0;33m";
constexpr const char *BLU = "\e[0;34m";
constexpr const char *MAG = "\e[0;35m";
constexpr const char *CYN = "\e[0;36m";
constexpr const char *WHT = "\e[0;37m";

// Regular bold text
constexpr const char *BBLK = "\e[1;30m";
constexpr const char *BRED = "\e[1;31m";
constexpr const char *BGRN = "\e[1;32m";
constexpr const char *BYEL = "\e[1;33m";
constexpr const char *BBLU = "\e[1;34m";
constexpr const char *BMAG = "\e[1;35m";
constexpr const char *BCYN = "\e[1;36m";
constexpr const char *BWHT = "\e[1;37m";

// Regular underline text
constexpr const char *UBLK = "\e[4;30m";
constexpr const char *URED = "\e[4;31m";
constexpr const char *UGRN = "\e[4;32m";
constexpr const char *UYEL = "\e[4;33m";
constexpr const char *UBLU = "\e[4;34m";
constexpr const char *UMAG = "\e[4;35m";
constexpr const char *UCYN = "\e[4;36m";
constexpr const char *UWHT = "\e[4;37m";

// Regular background
constexpr const char *BLKB = "\e[40m";
constexpr const char *REDB = "\e[41m";
constexpr const char *GRNB = "\e[42m";
constexpr const char *YELB = "\e[43m";
constexpr const char *BLUB = "\e[44m";
constexpr const char *MAGB = "\e[45m";
constexpr const char *CYNB = "\e[46m";
constexpr const char *WHTB = "\e[47m";

// High intensty background
constexpr const char *BLKHB = "\e[0;100m";
constexpr const char *REDHB = "\e[0;101m";
constexpr const char *GRNHB = "\e[0;102m";
constexpr const char *YELHB = "\e[0;103m";
constexpr const char *BLUHB = "\e[0;104m";
constexpr const char *MAGHB = "\e[0;105m";
constexpr const char *CYNHB = "\e[0;106m";
constexpr const char *WHTHB = "\e[0;107m";

// High intensty text
constexpr const char *HBLK = "\e[0;90m";
constexpr const char *HRED = "\e[0;91m";
constexpr const char *HGRN = "\e[0;92m";
constexpr const char *HYEL = "\e[0;93m";
constexpr const char *HBLU = "\e[0;94m";
constexpr const char *HMAG = "\e[0;95m";
constexpr const char *HCYN = "\e[0;96m";
constexpr const char *HWHT = "\e[0;97m";

// Bold high intensity text
constexpr const char *BHBLK = "\e[1;90m";
constexpr const char *BHRED = "\e[1;91m";
constexpr const char *BHGRN = "\e[1;92m";
constexpr const char *BHYEL = "\e[1;93m";
constexpr const char *BHBLU = "\e[1;94m";
constexpr const char *BHMAG = "\e[1;95m";
constexpr const char *BHCYN = "\e[1;96m";
constexpr const char *BHWHT = "\e[1;97m";

// Reset
constexpr const char *reset = "\e[0m";
constexpr const char *CRESET = "\e[0m";
constexpr const char *COLOR_RESET = "\e[0m";

} // namespace ansi_code
