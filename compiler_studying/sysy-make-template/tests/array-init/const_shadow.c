const int a[2]={5,6};int main(){int sum=a[1];{const int a[2]={1};sum=sum+a[0]+a[1];}return sum+a[0];}
