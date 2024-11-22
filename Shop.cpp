#include "Shop.h"

void Shop::init() {
   for (int i = 0; i < total_barbers; i++) {
      Barber* c = new Barber();
      myBarbers.push_back(c);
   }
   pthread_mutex_init(&mutex_, NULL);
   pthread_cond_init(&customers_waiting, NULL);
}

Shop::~Shop() {
   for (int i = 0; i < total_barbers; i++) {
      delete myBarbers[i];
   }
}

std::string Shop::int2string(int i) {
   std::stringstream out;
   out << i;
   return out.str();
}

void Shop::printBarber(int barber, std::string message) {
   std::cout << "barber  [" << barber << "]: " << message << std::endl;
}

void Shop::printCustomer(int customer, std::string message) {
   std::cout << "customer[" << customer << "]: " << message << std::endl;
}

int Shop::get_cust_drops() const {
   return cust_drops;
}

int Shop::visitShop(int id) {
   pthread_mutex_lock(&mutex_);

   // if all waiting chairs are full, then leave the shop
   if (waiting_chairs.size() == max_waiting_cust) {
      printCustomer(id, "leaves the shop because of no available waiting chairs.");
      cust_drops++;
      pthread_mutex_unlock(&mutex_);
      return -1;
   }

   int barber = -1;

   // if no barbers are available, then wait for a barber to become available
   if (available_barbers.empty()) {
      waiting_chairs.push(id);
      printCustomer(id, "takes a waiting chair. # waiting seats available = " +
                        int2string(max_waiting_cust - waiting_chairs.size()));
      pthread_cond_wait(&customers_waiting, &mutex_);
      barber = pairings[id];
   } else { // give any of the available barber to a customer
      barber = available_barbers.front();
      available_barbers.pop();
      pairings[id] = barber;
   }

   // give the active barber the customer by moving them to the chair
   printCustomer(id, "moves to a service chair[" + int2string(barber) + "], # waiting seats available = " +
                     int2string(max_waiting_cust - waiting_chairs.size()));
   myBarbers[barber]->current_customer = id;
   myBarbers[barber]->customer_in_chair = true;

   // wake up the barber just in case if they are sleeping, put them to work
   pthread_cond_signal(&myBarbers[pairings[id]]->awake_signal);
   myBarbers[barber]->in_service = true;
   
   pthread_mutex_unlock(&mutex_);
   return barber;
}

void Shop::leaveShop(int customer_id, int barber_id) {
   pthread_mutex_lock(&mutex_);

   // wait for service to complete before paying barber
   printCustomer(customer_id, "wait for barber[" + int2string(barber_id) + "] to be done with the hair-cut.");
   while (myBarbers[barber_id]->in_service) {
      pthread_cond_wait(&myBarbers[barber_id]->service_complete, &mutex_);
   }
   myBarbers[barber_id]->money_paid = true;

   // signal to the barber that they have been paid
   pthread_cond_signal(&myBarbers[barber_id]->paid_signal);
   printCustomer(customer_id, "says good-bye to barber[" + int2string(barber_id) + "].");
   pthread_mutex_unlock(&mutex_);
}

void Shop::helloCustomer(int barber_id) {
   pthread_mutex_lock(&mutex_);

   // if no customers, put the barber to sleep
   if (waiting_chairs.empty() && myBarbers[barber_id]->current_customer == -1) {
      printBarber(barber_id, "sleeps because of no customers.");
      available_barbers.push(barber_id);
      pthread_cond_wait(&myBarbers[barber_id]->awake_signal, &mutex_);
   }

   // wait for a customer to sit in the barber's chair
   while (!myBarbers[barber_id]->customer_in_chair) {
      pthread_cond_wait(&myBarbers[barber_id]->awake_signal, &mutex_);
   }

   // start the barber service
   myBarbers[barber_id]->in_service = true;
   printBarber(barber_id, "starts a hair-cut service for customer[" + int2string(myBarbers[barber_id]->current_customer) + "]");
   pthread_mutex_unlock(&mutex_);
}

void Shop::byeCustomer(int barber_id) {
   pthread_mutex_lock(&mutex_);

   // haircut is done and the barber is waiting for the payment, signal to the customer to pay
   myBarbers[barber_id]->in_service = false;
   printBarber(barber_id, "says he's done with a hair-cut service for customer [" + int2string(myBarbers[barber_id]->current_customer) + "]");
   myBarbers[barber_id]->money_paid = false;
   pthread_cond_signal(&myBarbers[barber_id]->service_complete);

   // wait while barber not paid
   while (!myBarbers[barber_id]->money_paid) {
      pthread_cond_wait(&myBarbers[barber_id]->paid_signal, &mutex_);
   }

   // barber is now available for a new customer
   myBarbers[barber_id]->customer_in_chair = false;
   pairings.erase(myBarbers[barber_id]->current_customer);
   myBarbers[barber_id]->current_customer = -1;
   
   // if waiting customers, assign to the barber that just finished
   printBarber(barber_id, "calls in another customer");
   if (!waiting_chairs.empty()) {
      int customer_id = waiting_chairs.front();
      waiting_chairs.pop();

      myBarbers[barber_id]->current_customer = customer_id;
      pairings[myBarbers[barber_id]->current_customer] = barber_id;
      myBarbers[barber_id]->in_service = true;

      // signal that a barber is available if no waiting chairs
      pthread_cond_signal(&customers_waiting);
   }

   pthread_mutex_unlock(&mutex_);
}
