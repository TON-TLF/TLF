# 定义对象文件的路径
OBPATH = Objects/

# 使用shell命令查找所有.cpp文件
CPPFILES = $(shell find . -name "*.cpp")

# 将所有.cpp文件的路径转换为.o文件的路径
# 例如，将 ./src/foo.cpp 转换为 objects/src/foo.o
OBJECTS_O = $(CPPFILES:./%.cpp=$(OBPATH)%.o)

# 将所有.cpp文件的路径转换为.d文件的路径（依赖文件）
OBJECTS_D = $(CPPFILES:./%.cpp=$(OBPATH)%.d)

# 定义C++编译器和编译选项
CXX = g++ -g -std=c++14 -O3
CXXFLAGS = -fpermissive -fopenmp -mpopcnt -mbmi2

# 目标：编译生成可执行文件main
# 依赖于所有的.o文件（即OBJECTS_O）
main: $(OBJECTS_O)
	@$(CXX) -o main $(OBJECTS_O) $(CXXFLAGS)
	@echo $(CXX) -o main $(OBJECTS_O) $(CXXFLAGS)

# 包含依赖文件（.d文件）
-include $(OBJECTS_D)

# 规则：生成依赖文件(.d)和对象文件(.o)
# 对于每个.d文件（由.cpp文件生成），遵循以下规则
$(OBJECTS_D): $(OBPATH)%.d : %.cpp
	@mkdir -p $(dir $@)
	$(CXX) -MM $(CXXFLAGS) $< > $@.$$$$
	sed 's,$(notdir $*.o):,$(OBPATH)$*.o:,g' $@.$$$$ > $@
	echo "	$(CXX) -c $< -o $(OBPATH)$*.o $(CXXFLAGS)" >> $@
	rm -f $@.$$$$

# 规则：编译.cpp文件生成.o文件
$(OBJECTS_O): $(OBPATH)%.o : %.cpp
	@mkdir -p $(dir $@)
	$(CXX) -c $< -o $@ $(CXXFLAGS)

# 伪目标：clean
.PHONY: clean

clean:
	rm -rf $(OBPATH)
	rm -rf main
	rm -rf res
