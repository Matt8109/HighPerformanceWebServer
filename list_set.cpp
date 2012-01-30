#include "list_set.hpp"

namespace base {

ListBasedSet::ListBasedSet() {
  head = NULL;
}

ListBasedSet::~ListBasedSet() {
}

bool ListBasedSet::insert(int value) {
  return true;
}

bool ListBasedSet::remove(int value) {
  return true;
}

bool ListBasedSet::lookup(int value) const {
  ListElement* current_element = head;

  while (current_element) {
    if (current_element->value < value)
      current_element = current_element->next; // Too cold
    else if (current_element->value == value)
      return true; // Just right
    else
      return false; // Too hot
  }
  
  return false;
}

void ListBasedSet::clear() {
  ListElement* current_node = head;
  ListElement* next_node = NULL;  

  if (!head)
    return;

   while (current_node) {
    // Store the next node before we delete
    next_node = head->next;
    
    delete current_node;

    current_node = next_node;
  }

  head = NULL;
}

bool ListBasedSet::checkIntegrity() const {
  bool is_ascending = true;
  int last_value = INT_MIN;
  ListElement* current_element = head;
 
  while (is_ascending && current_element) {
    if (current_element->value <= last_value) 
      is_ascending = false;

    last_value = current_element->value;
    current_element = current_element->next; 
  }  

  return is_ascending;
}


} // namespace base
