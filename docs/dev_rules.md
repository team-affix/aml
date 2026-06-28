no optional arguments
no optional template arguments
no overloading (prefer 1 function that receives all args from the generic case)
no public member vars UNLESS it is a value object
value objects can only have member functions if it helps with construction. all member functions should be private except for constructor and destructor
prefer only one constructor/destructor and avoid overloading. make any two functions that serve distinct purposes have different names
do not have default params ever. make the user specify them
use structs, never classes
only one outermost type per file; nested subtypes of that type are fine, but do not define unrelated types in the same file
for templated classes, put the struct definition first, and leave the member functions only declared. Then, the functions can be defined after the struct definition ends.
for templated classes, take separate template type parameters per collaborator capability (e.g. IMakeLcVar, ITranspileNat, ITranspileList) — do not bundle multiple roles into one template parameter. The caller may pass the same concrete object for every slot when one implementation provides all capabilities. Use strategy-style transpile interfaces so Scott vs Church is chosen by which transpiler is injected, not by branching inside the top-level transpiler.
prefer DI via references: structs should not own large collaborators with behavior (e.g. integer_transpiler holds a reference to scott_nat_transpiler supplied at construction). Value objects and small state are fine to own. (only exception being manifests)
do not expose code implementation unless necessary. So for any non-templatted classes, implement their member functions in a corresponding cpp file.
remove all explicit constructors. we do not keep those since it is typically unnecessary.
only use inheritance for antlr visitors. otherwise, we should just have templates for swapping-out behavior. (see atlas core for good examples of this)
prefer running the _fast debug variants for timely development testing
unit tests should only ever have the SUT concrete and all other dependencies mocked. (see how atlas does this)
integration tests should test how multiple combinations of concrete systems work together, unlike unit tests. SOME of the infra can still be mocked.
make sure that the code is as symmetrical as possible. keep things very organized.
PREFER SIMPLER CODE. Sometimes, we want DRY. Other times, we need to use less abstraction (examples: in the test files)
