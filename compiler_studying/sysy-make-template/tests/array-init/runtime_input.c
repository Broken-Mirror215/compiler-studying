int main() {
  int a[2][3] = {{getint()}, {getint(), getint()}};
  return a[0][0] * 100 + a[1][0] * 10 + a[1][1]
       + a[0][1] + a[0][2] + a[1][2];
}
