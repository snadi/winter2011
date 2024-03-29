#ifdef FOO
#define BAR
#ifdef BAR
void bar() {
    return;
}
#else
int bar() {
    return 1;
}
#endif
#endif

int main(void) {
    bar();
    return 0;
}

/*
 * check-name: Complex Conditions
 * check-command: undertaker -j cpppc $file
 * check-output-start
I: CPP Precondition for cpppc-cppcheck.c
( B0 <-> FOO )
&& ( B2 <->  ( B0 )  && BAR. )
&& ( B4 <->  ( B0 )  && ( ! (B2) )  )
&& (  ( (BAR) && !(B0) ) -> BAR. )
&& (  ( (BAR.)  && !(B0) ) -> BAR )
&& ( B0 -> BAR. )
&& B00
 * check-output-end
 */
