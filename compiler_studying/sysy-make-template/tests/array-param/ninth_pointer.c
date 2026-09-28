int f(int a,int b,int c,int d,int e,int h,int i,int j,int x[]) {
  return a+b+c+d+e+h+i+j+x[1];
}
int main() {
  int x[2]={0,9};
  return f(1,2,3,4,5,6,7,8,x);
}
