#pragma once

namespace infos {
namespace util {

class LinkedList
{
private:
    struct Node
    {
        void* data;
        Node* next;

        Node(void* data, Node* next = nullptr) : data(data), next(next) {}
    };

    Node* _head;

public:
    LinkedList() : _head(nullptr) {}

    void clear()
    {
        while (_head)
        {
            Node* tmp = _head;
            _head = _head->next;
            delete tmp;
        }
    }

    void append(void* data)
    {
        Node* new_node = new Node(data);
        if (!_head)
        {
            _head = new_node;
        }
        else
        {
            Node* current = _head;
            while (current->next)
                current = current->next;
            current->next = new_node;
        }
    }

    void* remove_first()
    {
        if (!_head) return nullptr;
        Node* tmp = _head;
        _head = _head->next;
        void* data = tmp->data;
        delete tmp;
        return data;
    }

    void* first() const
    {
        return _head ? _head->data : nullptr;
    }

    void* next(void* current_data) const
    {
        Node* current = _head;
        while (current)
        {
            if (current->data == current_data)
                return current->next ? current->next->data : nullptr;
            current = current->next;
        }
        return nullptr;
    }

    void remove(void* prev_data, void* target_data)
    {
        if (!_head) return;

        if (!_head->next && _head->data == target_data)
        {
            delete _head;
            _head = nullptr;
            return;
        }

        Node* prev = nullptr;
        Node* current = _head;

        while (current)
        {
            if (current->data == target_data)
            {
                if (prev)
                {
                    prev->next = current->next;
                }
                else
                {
                    _head = current->next;
                }
                delete current;
                return;
            }
            prev = current;
            current = current->next;
        }
    }

    bool empty() const
    {
        return _head == nullptr;
    }
};

}
}
