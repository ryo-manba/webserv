#ifndef TEST_COMMON_HPP
# define TEST_COMMON_HPP

# include <iostream>
# include <string>
# include <cstdlib>
# define DSOUT() debug_out(__FILE__, __LINE__)
# define DOUT()  debug_err(__FILE__, __LINE__)

std::ostream&   debug_out(
    const char *filename,
    const int linenumber
);

std::ostream&   debug_err(
    const char *filename,
    const int linenumber
);

#endif
