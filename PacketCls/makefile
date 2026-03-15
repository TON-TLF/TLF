OBPATH = Objects/
CPPFILES = $(shell find . -name "*.cpp")
OBJECTS_O = $(CPPFILES:./%.cpp=$(OBPATH)%.o)
OBJECTS_D = $(CPPFILES:./%.cpp=$(OBPATH)%.d)

CXX = g++ -g -std=c++14 -O3
CXXFLAGS = -fpermissive -fopenmp -mpopcnt -mbmi2

main: $(OBJECTS_O)
	@$(CXX) -o main $(OBJECTS_O) $(CXXFLAGS)
	@echo $(CXX) -o main $(OBJECTS_O) $(CXXFLAGS)

-include $(OBJECTS_D)

$(OBJECTS_D): $(OBPATH)%.d : %.cpp
	@mkdir -p $(dir $@)
	$(CXX) -MM $(CXXFLAGS) $< > $@.$$$$
	sed 's,$(notdir $*.o):,$(OBPATH)$*.o:,g' $@.$$$$ > $@
	echo "	$(CXX) -c $< -o $(OBPATH)$*.o $(CXXFLAGS)" >> $@
	rm -f $@.$$$$

$(OBJECTS_O): $(OBPATH)%.o : %.cpp
	@mkdir -p $(dir $@)
	$(CXX) -c $< -o $@ $(CXXFLAGS)

.PHONY: clean

clean:
	rm -rf $(OBPATH)
	rm -rf main
	rm -rf res
