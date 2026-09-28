int ticks;
int mark(int x) {
  ticks = ticks * 10 + x;
  return x + 10;
}
int main() {
  int a[2][3] = {{mark(1)}, {mark(2), mark(3)}};
  return ticks + a[0][0] + a[0][1] + a[0][2]
       + a[1][0] + a[1][1] + a[1][2];
}
