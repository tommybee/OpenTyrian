void
foo (void)
{
  const char *s;
  s = __FUNCTION__; /* { dg-warning " ISO C does not support .__FUNCTION__. predefined identifier" } */
  s = __PRETTY_FUNCTION__; /* { dg-warning " ISO C does not support .__PRETTY_FUNCTION__. predefined identifier" } */
  s = __extension__ __FUNCTION__;
  s = __extension__ __PRETTY_FUNCTION__;
}
