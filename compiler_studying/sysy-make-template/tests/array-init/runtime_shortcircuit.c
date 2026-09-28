int hits = 0;
int bump() {
  hits = hits + 1;
  return hits;
}
int main() {
  int a[2][3] = {{0 && bump(), 1 || bump(), 1 && bump()}, {0 || bump()}};
  return hits * 10 + a[0][0] + a[0][1] + a[0][2]
       + a[1][0] + a[1][1] + a[1][2];
}
