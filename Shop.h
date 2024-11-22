#ifndef SHOP_H_
#define SHOP_H_

#include <pthread.h>
#include <iostream>
#include <sstream>
#include <string>
#include <queue>
#include <map>
using namespace std;

#define kDefaultNumChairs 3
#define kDefaultNumBarbers 1

class Shop {
public:
   Shop(int num_barbers, int num_chairs)
      : max_waiting_cust(num_chairs),
         total_barbers(num_barbers),
         cust_drops(0) {
      init();
   }

   Shop()
      : max_waiting_cust(kDefaultNumChairs),
         total_barbers(kDefaultNumBarbers),
         cust_drops(0) {
      init();
   }

   ~Shop();

   int visitShop(int customer_id);
   void leaveShop(int customer_id, int barber_id);
   void helloCustomer(int barber_id);
   void byeCustomer(int barber_id);
   int get_cust_drops() const;

private:
   struct Barber {
      pthread_cond_t paid_signal;
      pthread_cond_t awake_signal;
      pthread_cond_t service_complete;
      int current_customer = -1;
      bool money_paid = false;
      bool in_service = false;
      bool customer_in_chair = false;

      Barber() {
         pthread_cond_init(&paid_signal, NULL);
         pthread_cond_init(&awake_signal, NULL);
         pthread_cond_init(&service_complete, NULL);
      }
   };
   
   vector<Barber*> myBarbers;
   const int max_waiting_cust;
   int total_barbers;
   queue<int> waiting_chairs;
   queue<int> available_barbers;
   map<int, int> pairings;
   int cust_drops;

   pthread_mutex_t mutex_;
   pthread_cond_t customers_waiting; 

   void init();
   string int2string(int i);
   void printBarber(int barber_id, string message);
   void printCustomer(int customer_id, string message);
};

#endif
