#include<stdio.h>
void codeOnly(int a, int b ){
   int c = a+b;
   printf("%d\n",c);
   return;
}

int dataOnly(int a, int b ){
   int c = a+b;
   return c;
}

int codeAndData(int a, int b ){
   int c = a+b;
   printf("%d\n",c);
   return c;
}

int c=0;
int main(){
  int a =1;
  int b =2;
  // 仅代码泄漏
  for(int i=0;i<5;i++)
    codeOnly(a,b);

  // 仅数据泄漏
  for(int i=0;i<7;i++){
    c+=dataOnly(a,b);
    printf("%d\n",c);
  }

  printf("after %d\n",c);
    
  // 数据和代码都存在泄漏
  for(int i=0;i<7;i++)
    c+=codeAndData(a,c);
  
  return 0; 
}
