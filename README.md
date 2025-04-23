# nwss-cnc

## next steps

- [X] Transform paths to bed+material size (maintain aspect ratio)
- [X] Optimize paths in cases where theres uniform circles or straight lines (instead of going to 100 points sequentially it could be one GCODE command)
- [X] GCODE generator from positions + config
- [X] load from + save to config file
- [ ] Create onboarding CLI
- [ ] Write documentation
- [X] Improve discretization algorithm
- [ ] Add modes (direct cutout, etch, etc)
- [ ] Add time estimation based on feed rate and number of passes?
- [ ] GCode optimizations
- [ ] IF we can communicate with the machine, add GCode upload and run options, add manual control and debug info like spindle speed and position + current line of program excecution, also e stop
- [ ] GUI for improved ux
- [ ] Create presentation