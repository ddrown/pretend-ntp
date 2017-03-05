#include <pthread.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/time.h>

#define THREADS 4
#define UPDATE_SPEED 100000

struct thread_info {
  pthread_t thread_id;
  int thread_num;
  uint32_t changes_seen;
};

#define STATES_COUNT 10
struct current_state {
  struct timeval when;
  uint32_t counter;
  int done;
} states[STATES_COUNT] = { {.done = 0}, {.done = 0} };
volatile uint32_t states_i = 0;

static void *creator_thread(void *arg) {
  uint32_t used_state = states_i;
  uint32_t free_state = (states_i + 1) % STATES_COUNT;
  uint32_t counter = 0;
  unsigned int rand_seed = time(NULL) ^ getpid();

  printf("creator thread start\n");
  for(uint32_t i = 0; i < 20; i++) {
    usleep(rand_r(&rand_seed) % UPDATE_SPEED);

    gettimeofday(&states[free_state].when, NULL);
    states[free_state].counter = counter++;

    printf("#W changing to %u at %ld.%06ld\n", states[free_state].counter, 
      states[free_state].when.tv_sec, states[free_state].when.tv_usec);
    // only this thread is allowed to change the values states_i and states
    assert(__sync_val_compare_and_swap(&states_i, used_state, free_state) == used_state);
    __sync_synchronize();

    used_state = free_state;
    free_state = (used_state + 1) % STATES_COUNT;
  }

  states[free_state].done = 1;
  gettimeofday(&states[free_state].when, NULL);
  states[free_state].counter = counter;

  // only this thread is allowed to change the values states_i and states
  assert(__sync_val_compare_and_swap(&states_i, used_state, free_state) == used_state);
  __sync_synchronize();

  printf("creator thread done at %ld.%06ld\n", states[free_state].when.tv_sec, states[free_state].when.tv_usec);

  return NULL;
}

static void *consumer_thread(void *arg) {
  struct thread_info *tinfo = arg;
  uint32_t last_counter = 0;
  struct timeval now, diff;

  tinfo->changes_seen = 0;

  while(states[states_i].done == 0) {
    if(last_counter != states[states_i].counter) {
      gettimeofday(&now, NULL);
      timersub(&now, &states[states_i].when, &diff);
      printf("#R%d counter change from %u to %u at %ld.%06ld (%ld.%06ld)\n", tinfo->thread_num, last_counter,
        states[states_i].counter, 
        states[states_i].when.tv_sec, states[states_i].when.tv_usec, diff.tv_sec, diff.tv_usec);
      last_counter = states[states_i].counter;
      tinfo->changes_seen++;
    }
  }

  printf("#R%d done saw %u\n", tinfo->thread_num, tinfo->changes_seen);
  return NULL;
}

int main() {
  struct thread_info tinfo[THREADS];
  int s;
  pthread_t writer_thread;
  void *res;

  s = pthread_create(&writer_thread, NULL, &creator_thread, NULL);
  if (s != 0) {
    errno = s;
    perror("pthread_create(writer) failed");
    exit(EXIT_FAILURE);
  }

  for (uint32_t i = 0; i < THREADS; i++) {
    tinfo[i].thread_num = i;
    s = pthread_create(&tinfo[i].thread_id, NULL, &consumer_thread, &tinfo[i]);
    if (s != 0) {
      errno = s;
      perror("pthread_create(reader) failed");
      exit(EXIT_FAILURE);
    }
  }

  printf("threads created\n");

  s = pthread_join(writer_thread, &res);
  if (s != 0) {
    errno = s;
    perror("pthread_join(writer) failed");
    exit(EXIT_FAILURE);
  }
  printf("writer done\n");

  for(uint32_t i = 0; i < THREADS; i++) {
    s = pthread_join(tinfo[i].thread_id, &res);
    if (s != 0) {
      errno = s;
      perror("pthread_join(reader) failed");
      exit(EXIT_FAILURE);
    }

    printf("reader thread %d done\n", tinfo[i].thread_num);
  }

  exit(EXIT_SUCCESS);
}
