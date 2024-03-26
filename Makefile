TEST_BINARY := test

run: $(TEST_BINARY)
	./$< && echo "\033[32mOK\033[0m" || echo "\033[31mFAIL\033[0m"

$(TEST_BINARY): buffer.h myalloc.h myalloc.cpp test.cpp
	g++ -std=c++03 -Wall -Werror $^ -o $@

clean:
	rm $(TEST_BINARY)
