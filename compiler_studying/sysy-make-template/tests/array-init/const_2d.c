const int n = 3;
const int g[2][n] = {{1}, {2, 3}};
int main() {
  const int a[2][3] = {{4}, {5, 6}};
  return g[0][0] + g[0][2] + g[1][0] + g[1][1] + g[1][2]
       + a[0][0] + a[0][2] + a[1][0] + a[1][1] + a[1][2];
}
