#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <math.h>

static size_t page_size;

// align_down - rounds a value down to an alignment
// @x: the value
// @a: the alignment (must be power of 2)
//
// Returns an aligned value.
#define align_down(x, a) ((x) & ~((typeof(x))(a) - 1))

#define AS_LIMIT	(1 << 25) // Maximum limit on virtual memory bytes
#define MAX_SQRTS	(1 << 27) // Maximum limit on sqrt table entries
#define MAX_FAULTS (1 << 12) // Maximum number of pages we can map before the first need for an unmap.
// if we reduce the number of MAX_FAULTS, the number of faults encountered increases.
int max_faults = 1;
int as_limit = 1 << 25;
struct node { // Used to make a node of the queue (Linked list application of the queue)
  int poses[2];
  uintptr_t page_number;  // Used as value
  struct node* next;    // Next Pointer
};

struct node* page_queue_front= NULL; // Rear and front pointers to keep the track of the queue's start and end
struct node* page_queue_rear= NULL;

int faults_encountered = 0; // Stores the number of page faults we faced.
static double *sqrts;
uintptr_t prev_fault_addr;  // Stores the address of a number (prev number actually) whose page is valid. We will evacuate this page once number of faults exceeds MAX_FAULTS 
// Use this helper function as an oracle for square root values.


void push_Back(uintptr_t value, int left, int right) // FIFO(Queue) function to insert nodes/ elements at the end to the queue
{
  struct node *temp;
  temp=(struct node *)malloc(sizeof(struct node));
  if(temp==NULL)
  {
    printf("No Memory available\n");
    exit(0);
  }
  temp->page_number = value;
  temp->next=NULL;
  temp->poses[0] = left;
  temp->poses[1] = right;
  if(page_queue_front == NULL){
    page_queue_front  = temp;
    page_queue_rear = temp;
  }else{
    page_queue_rear ->next = temp;
    page_queue_rear = temp;
  }
  return;
}

uintptr_t pop_Front() // Function to remove the the front element from the queue
{
  struct node *temp;
  uintptr_t pg_number;

  if(page_queue_front == NULL && page_queue_rear == NULL)
  {
    printf("The queue is empty can not delete Error\n");
    exit(0);
  }

  temp = page_queue_front;
  page_queue_front = page_queue_front->next;
  pg_number = temp->page_number;
  
  free(temp);
  return pg_number;
  
  
}

static void
calculate_sqrts(double *sqrt_pos, int start, int nr)
{
  int i;

  for (i = 0; i < nr; i++)
    sqrt_pos[i] = sqrt((double)(start + i));
}


// We have implemented a queue to keep the number of faults in bound and if they go beyond the bound we remove the first element(pop_Front)
//  we remove the element (first entered page) from the queue and unmap it, and insert when a new page is added.
static void
printTable(){
  int num = 1;
  struct node* temp = page_queue_front;
  printf("Current status of the table\n");
  while (temp != page_queue_rear){
    printf("\t %d positon range = [%d,%d]\n",num,temp->poses[0],temp->poses[1]);
    temp = temp->next;
    num += 1;
  }
  if (num > 1){
    printf("\t %d positon range = [%d,%d]\n",num,page_queue_rear->poses[0],page_queue_rear->poses[1]);
  }
  printf("----------------\n");
}

static void
handle_sigsegv(int sig, siginfo_t *si, void *ctx)
{
  //printTable();
  uintptr_t fault_addr = (uintptr_t)si->si_addr;
  uintptr_t req_page_number = align_down(fault_addr, page_size); // page number of the page in which the req sqrt[pos] is present.
  faults_encountered += 1;                                       // increasing the counter each time we enter the signal handler
  if (faults_encountered > max_faults){   
    uintptr_t prev_page_number= pop_Front();
    if (munmap((void*)prev_page_number, page_size) == -1) {
      fprintf(stderr, "Couldn't munmap() the previous page; %s\n",
              strerror(errno));
      exit(EXIT_FAILURE);
    }
  }
  
  double *mapped_page = mmap((void*)req_page_number, page_size, PROT_READ | PROT_WRITE,  // Newly mapped page
                          MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);

  if (mapped_page == MAP_FAILED) {
    fprintf(stderr, "Couldn't mmap() a new page for square root values at 0x%lx; %s\n",
            req_page_number, strerror(errno));
    exit(EXIT_FAILURE);
  }
  int pos = (req_page_number / sizeof(double)) - ((uintptr_t)sqrts/ sizeof(double)); // Found the position wrt to the alloted memory(according to the setup_sqrt_region funtion), sqrts is pointed to it.
  int nums_in_page = page_size / sizeof(double);
  calculate_sqrts(mapped_page, pos, nums_in_page);
  int offset_pg = pos/nums_in_page; 
  int posl = offset_pg*nums_in_page;
  int posr = posl + nums_in_page - 1;
  push_Back(req_page_number, posl, posr);
  return;
}

static void
setup_sqrt_region(void)
{
  struct rlimit lim = {as_limit, as_limit};
  struct sigaction act;

  // Only mapping to find a safe location for the table.
  sqrts = mmap(NULL, MAX_SQRTS * sizeof(double) + as_limit, PROT_NONE,
	       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (sqrts == MAP_FAILED) {
    fprintf(stderr, "Couldn't mmap() region for sqrt table; %s\n",
	    strerror(errno));
    exit(EXIT_FAILURE);
  }

  // Now release the virtual memory to remain under the rlimit.
  if (munmap(sqrts, MAX_SQRTS * sizeof(double) + as_limit) == -1) {
    fprintf(stderr, "Couldn't munmap() region for sqrt table; %s\n",
            strerror(errno));
    exit(EXIT_FAILURE);
  }

  // Set a soft rlimit on virtual address-space bytes.
  if (setrlimit(RLIMIT_AS, &lim) == -1) {
    fprintf(stderr, "Couldn't set rlimit on RLIMIT_AS; %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }

  // Register a signal handler to capture SIGSEGV.
  act.sa_sigaction = handle_sigsegv;
  act.sa_flags = SA_SIGINFO;
  sigemptyset(&act.sa_mask);
  if (sigaction(SIGSEGV, &act, NULL) == -1) {
    fprintf(stderr, "Couldn't set up SIGSEGV handler;, %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }
}

static void
test_sqrt_region1(void)
{
  int i, pos = rand() % (MAX_SQRTS - 1);
  double correct_sqrt;

  printf("Validating square root table contents...\n");
  srand(0xDEADBEEF);

  for (i = 0; i < 500000; i++) {
    if (i % 2 == 0)
      pos = rand() % (MAX_SQRTS - 1);
    else
      pos += 1;
    calculate_sqrts(&correct_sqrt, pos, 1);
    if (sqrts[pos] != correct_sqrt) {
      fprintf(stderr, "Square root is incorrect. Expected %f, got %f.\n",
              correct_sqrt, sqrts[pos]);
      exit(EXIT_FAILURE);
    }
  }

  printf("All tests passed!\n");
  printf("Total page faults encountered :%d\n",faults_encountered);
}
static void
test_sqrt_region2(void)
{
  int i, pos = rand() % (MAX_SQRTS - 1);
  double correct_sqrt;

  printf("Validating square root table contents...\n");
  srand(0xDEADBEEF);

  
for (i = 0; i < 50000000; i++)
{
    pos = i % (MAX_SQRTS - 1);
    calculate_sqrts(&correct_sqrt, pos, 1);
    if (sqrts[pos] != correct_sqrt) {
      fprintf(stderr, "Square root is incorrect. Expected %f, got %f.\n",
              correct_sqrt, sqrts[pos]);
      exit(EXIT_FAILURE);
    }
  }

  printf("All tests passed!\n");
  printf("Total page faults encountered :%d\n",faults_encountered);
}

static void
test_sqrt_region3(void)
{
  int i, pos = rand() % (MAX_SQRTS - 1);
  double correct_sqrt;

  printf("Validating square root table contents...\n");
  srand(0xDEADBEEF);

  
for (i = 0; i < 50000000; i = i + 1000)
{
  pos = i % (MAX_SQRTS - 1);
  calculate_sqrts(&correct_sqrt, pos, 1);
  if (sqrts[pos] != correct_sqrt){
      fprintf(stderr, "Square root is incorrect. Expected %f, got %f.\n",
              correct_sqrt, sqrts[pos]);
      exit(EXIT_FAILURE);
    }
  }

  printf("All tests passed!\n");
  printf("Total page faults encountered :%d\n",faults_encountered);
}

int
main(int argc, char *argv[])
{
  int N = atoi(argv[1]);
  max_faults = 1 << N; 
  if (N > 12){
    as_limit = 1 << N + 13;
  }
  else {
    as_limit = 1 << 25;
  }
  page_size = sysconf(_SC_PAGESIZE);
  printf("page_size is %ld\n", page_size);
  setup_sqrt_region();
  test_sqrt_region1();
  return 0;
}