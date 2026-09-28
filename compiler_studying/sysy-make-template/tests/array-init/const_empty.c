const int g[2][2] = {{}, {}};
int main() {
  const int a[2][3][4] = {};
  return g[0][0] + g[1][1] + a[0][2][3] + a[1][2][3];
}
