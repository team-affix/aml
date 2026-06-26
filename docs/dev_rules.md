no optional arguments
no optional template arguments
no overloading (prefer 1 function that receives all args from the generic case)
no public member vars UNLESS its a value object
prefer running the _fast debug variants for timely development testing
unit tests should only ever have the SUT concrete and all other dependencies mocked. (see how atlas does this)
integration tests should test how multiple combinations of concrete systems work together, unlike unit tests. SOME of the infra can still be mocked.
make sure that the code is as symmetrical as possible. keep things very organized.
PREFER SIMPLER CODE. Sometimes, we want DRY. Other times, we need to use less abstraction (examples: in the test files)
ANALYZE THE CODEBASE AND IDEATE IF IT COULD BE MADE BETTER IN SOME MASSIVE WAY. MAKE IT BETTER IF SO. LIKE HUGE REFACTOR IS FINE
