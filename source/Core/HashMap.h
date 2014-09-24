#pragma once

#include "Core/Memory.h"
#include "Core/Slice.h"
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <type_traits>

template <typename K, typename V>
struct KeyAndValue {
	K key;
	V value;
};

template <typename K, typename V>
struct HashMap {
	static constexpr float LOAD = 6.5;
	static constexpr int BUCKET_SIZE = 8;
	static constexpr int MAX_KEY_SIZE = 128;
	static constexpr int MAX_VALUE_SIZE = 128;
	static constexpr bool INDIRECT_KEY = sizeof(K) > MAX_KEY_SIZE;
	static constexpr bool INDIRECT_VALUE = sizeof(V) > MAX_VALUE_SIZE;
	typedef typename std::conditional<INDIRECT_KEY, K*, K>::type IK;
	typedef typename std::conditional<INDIRECT_VALUE, V*, V>::type IV;

	using KeyType = K;
	using ValueType = V;

	template <bool Indirect, typename T>
	struct _Indirect;

	// when T is indirectly stored, all functions take T*
	template <typename T>
	struct _Indirect<true, T> {
		static T &get(T *a) { return *a; }
		static void destroy(T *a) { a->~T(); free_memory(a); }
		static T *insert(T *a) { *a = allocate_memory<T>(); return *a; }
	};

	// when T is directly stored, all functions take T&
	template <typename T>
	struct _Indirect<false, T>
	{
		static T &get(T &a) { return a; }
		static void destroy(T &a) { a.~T(); }
		static T *insert(T &a) { return &a; }
	};

	struct Bucket {
		uint8_t top_hash[BUCKET_SIZE];
		Bucket *overflow;
		IK keys[BUCKET_SIZE];
		IV values[BUCKET_SIZE];

		K &key(int i) { return _Indirect<INDIRECT_KEY, K>::get(keys[i]); }
		V &value(int i) { return _Indirect<INDIRECT_VALUE, V>::get(values[i]); }
		void clear()
		{
			memset(top_hash, 0, BUCKET_SIZE);
			overflow = nullptr;
		}

		void free()
		{
			for (int i = 0; i < BUCKET_SIZE; i++) {
				if (top_hash[i] == 0)
					continue;

				_Indirect<INDIRECT_KEY, K>::destroy(keys[i]);
				_Indirect<INDIRECT_VALUE, V>::destroy(values[i]);
			}
		}
	};

	int m_count;
	uint8_t m_B;
	Bucket *m_buckets;

	void _split_bucket(Bucket *b, int i)
	{
		int newbit = 1 << (m_B - 1);
		Bucket *x = m_buckets + i;
		Bucket *y = m_buckets + i + newbit;
		x->clear();
		y->clear();
		int xi = 0;
		int yi = 0;

		do {
			for (int i = 0; i < BUCKET_SIZE; i++) {
				if (b->top_hash[i] == 0)
					continue;

				int hash = compute_hash(b->key(i));
				if ((hash & newbit) == 0) {
					if (xi == BUCKET_SIZE) {
						Bucket *newx = allocate_memory<Bucket>();
						newx->clear();
						x->overflow = newx;
						x = newx;
						xi = 0;
					}
					x->top_hash[xi] = b->top_hash[i];
					new (&x->keys[xi]) IK(std::move(b->keys[i]));
					new (&x->values[xi]) IV(std::move(b->values[i]));
					xi++;
				} else {
					if (yi == BUCKET_SIZE) {
						Bucket *newy = allocate_memory<Bucket>();
						newy->clear();
						y->overflow = newy;
						y = newy;
						yi = 0;
					}
					y->top_hash[yi] = b->top_hash[i];
					new (&y->keys[yi]) IK(std::move(b->keys[i]));
					new (&y->values[yi]) IV(std::move(b->values[i]));
					yi++;
				}
			}
			b = b->overflow;
		} while (b);
	}

	void _grow()
	{
		Bucket *old_buckets = m_buckets;
		int old_buckets_n = 1 << m_B;

		m_B++;
		m_buckets = allocate_memory<Bucket>(1 << m_B);

		for (int i = 0; i < old_buckets_n; i++) {
			Bucket *b = old_buckets + i;
			_split_bucket(b, i);

			// free old bucket contents and overflow buckets
			Bucket *next = b->overflow;
			while (next) {
				Bucket *cur = next;
				next = cur->overflow;

				cur->free();
				free_memory(cur);
			}
			b->free();
		}
		free_memory(old_buckets);
	}

	void _dump()
	{
		printf("Dumping map info with %d elements\n", m_count);
		const int buckets_n = 1 << m_B;
		printf("Number of buckets: %d\n", buckets_n);
		int overflows_n = 0;
		for (int i = 0; i < buckets_n; i++) {
			Bucket *b = m_buckets + i;
			while (b->overflow) {
				overflows_n++;
				b = b->overflow;
			}
		}
		printf("Number of overflow buckets: %d\n", overflows_n);

		int items_in_root = 0;
		int items_in_overflow = 0;
		for (int i = 0; i < buckets_n; i++) {
			Bucket *b = m_buckets + i;
			for (int i = 0; i < BUCKET_SIZE; i++) {
				if (b->top_hash[i] != 0)
					items_in_root++;
			}
			while (b->overflow) {
				b = b->overflow;
				for (int i = 0; i < BUCKET_SIZE; i++) {
					if (b->top_hash[i] != 0)
						items_in_overflow++;
				}
			}
		}
		printf("Number of items in root buckets: %d\n", items_in_root);
		printf("Number of items in overflow buckets: %d\n", items_in_overflow);
	}

	template <typename K2>
	IV *_lookup(const K2 &key,
		IK **key_out = nullptr, uint8_t **top_out = nullptr) const
	{
		if (m_count == 0)
			return nullptr;

		int hash = compute_hash(key);
		int bi = hash & ((1 << m_B) - 1);
		Bucket *b = m_buckets + bi;
		uint8_t top = hash >> (sizeof(int) * 8 - 8);
		if (top == 0)
			top = 1;

		for (;;) {
			for (int i = 0; i < BUCKET_SIZE; i++) {
				if (b->top_hash[i] != top)
					continue;

				if (!(key == b->key(i)))
					continue;

				if (top_out)
					*top_out = &b->top_hash[i];
				if (key_out)
					*key_out = &b->keys[i];
				return &b->values[i];
			}

			if (b->overflow == nullptr)
				break;
			b = b->overflow;
		}
		return nullptr;
	}

	void _Nullify()
	{
		m_count = 0;
		m_B = 0;
		m_buckets = nullptr;
	}

	explicit HashMap(int hint = 0)
	{
		m_count = 0;
		m_B = 0;
		m_buckets = nullptr;

		while (hint > BUCKET_SIZE && hint > LOAD * (1 << m_B))
			m_B++;

		if (m_B != 0) {
			m_buckets = allocate_memory<Bucket>(1 << m_B);
			for (int i = 0, n = 1 << m_B; i < n; i++)
				m_buckets[i].clear();
		}
	}

	HashMap(const HashMap&) = delete;
	HashMap(HashMap &&r): m_count(r.m_count), m_B(r.m_B), m_buckets(r.m_buckets)
	{
		r._Nullify();
	}

	HashMap &operator=(const HashMap &r) = delete;
	HashMap &operator=(HashMap &&r)
	{
		this->~HashMap();
		m_count = r.m_count;
		m_B = r.m_B;
		m_buckets = r.m_buckets;
		r._Nullify();
		return *this;
	}

	~HashMap()
	{
		if (m_buckets == nullptr)
			return;

		clear();
		free_memory(m_buckets);
	}

	template <typename K2>
	V *get(const K2 &key)
	{
		IV *v = _lookup(key);
		return v ? &_Indirect<INDIRECT_VALUE, V>::get(*v) : nullptr;
	}

	template <typename K2>
	const V *get(const K2 &key) const
	{
		IV *v = _lookup(key);
		return v ? &_Indirect<INDIRECT_VALUE, V>::get(*v) : nullptr;
	}

	// just a syntax sugar
	template <typename K2>
	V &operator[](const K2 &key)
	{
		V *v = get(key);
		NG_ASSERT(v != nullptr);
		return *v;
	}
	template <typename K2>
	const V &operator[](const K2 &key) const
	{
		const V *v = get(key);
		NG_ASSERT(v != nullptr);
		return *v;
	}

	template <typename K2>
	V get_or_default(const K2 &key, V def) const
	{
		IV *v = _lookup(key);
		return v ? _Indirect<INDIRECT_VALUE, V>::get(*v) : def;
	}

	template <typename K2>
	void remove(const K2 &key)
	{
		uint8_t *top;
		IK *ik;
		IV *iv = _lookup(key, &ik, &top);
		if (!iv)
			return;

		*top = 0;
		_Indirect<INDIRECT_KEY, K>::destroy(*ik);
		_Indirect<INDIRECT_VALUE, V>::destroy(*iv);
		m_count--;
	}

	void clear()
	{
		if (m_buckets == nullptr)
			return;

		for (int i = 0, n = 1 << m_B; i < n; i++) {
			Bucket &b = m_buckets[i];
			Bucket *next = b.overflow;
			while (next) {
				Bucket *cur = next;
				next = cur->overflow;

				cur->free();
				free_memory(cur);
			}
			b.free();
			b.clear();
		}
		m_count = 0;
	}

	template <typename KK, typename VV>
	V *insert(KK &&key, VV &&value)
	{
		int hash = compute_hash(key);
		if (m_buckets == nullptr) {
			m_buckets = allocate_memory<Bucket>();
			m_buckets->clear();
		}

again:
		int bi = hash & ((1 << m_B) - 1);
		Bucket *b = m_buckets + bi;
		uint8_t top = hash >> (sizeof(int) * 8 - 8);
		if (top == 0)
			top = 1;

		uint8_t *insert_top = nullptr;
		IK *insert_key = nullptr;
		IV *insert_value = nullptr;

		for (;;) {
			for (int i = 0; i < BUCKET_SIZE; i++) {
				if (b->top_hash[i] != top) {
					if (b->top_hash[i] == 0 && insert_top == nullptr) {
						insert_top = b->top_hash + i;
						insert_key = b->keys + i;
						insert_value = b->values + i;
					}
					continue;
				}

				if (key != b->key(i))
					continue;
				V &val = b->value(i);
				val = std::forward<VV>(value);
				return &val;
			}

			if (b->overflow == nullptr)
				break;
			b = b->overflow;
		}

		if (m_count >= LOAD * (1 << m_B) && m_count >= BUCKET_SIZE) {
			_grow();
			goto again;
		}

		if (insert_top == nullptr) {
			Bucket *newb = allocate_memory<Bucket>();
			newb->clear();
			b->overflow = newb;
			insert_top = newb->top_hash;
			insert_key = &newb->key(0);
			insert_value = &newb->value(0);
		}

		K *ik = _Indirect<INDIRECT_KEY, K>::insert(*insert_key);
		V *iv = _Indirect<INDIRECT_VALUE, V>::insert(*insert_value);

		*insert_top = top;
		new (ik) K(std::forward<KK>(key));
		new (iv) V(std::forward<VV>(value));
		m_count++;
		return iv;
	}

	int length() const { return m_count; }
	int capacity() const { return LOAD * (1 << m_B); }
};

template <typename T>
struct HashMapIter {
	using K = typename T::KeyType;
	using V = typename T::ValueType;
	typename T::Bucket *m_buckets = nullptr;
	typename T::Bucket *m_bucket = nullptr;
	int m_buckets_n = 0;
	int m_bucket_i = 0;
	int m_i = 0;

	void _find_next_valid()
	{
		if (m_bucket_i == m_buckets_n) {
			return;
		}

		for (;;) {
			m_i++;
			if (m_i == T::BUCKET_SIZE) {
				// done with current bucket, try next overflow
				m_bucket = m_bucket->overflow;
				m_i = 0;
				if (m_bucket == nullptr) {
					// oops, no overflow bucket, try next
					// one in the table
					m_bucket_i++;
					if (m_bucket_i == m_buckets_n) {
						// no buckets left, we're done
						return;
					}
					m_bucket = m_buckets + m_bucket_i;
				}
			}

			// check if it's a valid entry
			if (m_bucket->top_hash[m_i] != 0) {
				return;
			}
		}
	}

	HashMapIter &operator++()
	{
		_find_next_valid();
		return *this;
	}

	bool operator==(const HashMapIter&) const { return m_bucket_i == m_buckets_n; }
	bool operator!=(const HashMapIter&) const { return m_bucket_i != m_buckets_n; }
	KeyAndValue<K&, V&> operator*()
	{
		return {
			T::template _Indirect<
				T::INDIRECT_KEY, K
			>::get(m_bucket->keys[m_i]),
			T::template _Indirect<
				T::INDIRECT_VALUE, V
			>::get(m_bucket->values[m_i]),
		};
	}

	HashMapIter() = default;

	HashMapIter(T &map):
		m_buckets(map.m_buckets),
		m_bucket(map.m_buckets),
		m_buckets_n(1 << map.m_B),
		m_bucket_i(0),
		m_i(0)
	{
		if (map.length() == 0) {
			m_buckets_n = 0;
			return;
		}
		if (m_bucket->top_hash[m_i] == 0)
			_find_next_valid();
	}
};

template <typename K, typename V>
HashMapIter<HashMap<K, V>> begin(HashMap<K, V> &m)
{
	return HashMapIter<HashMap<K, V>>(m);
}

template <typename K, typename V>
HashMapIter<HashMap<K, V>> end(HashMap<K, V>&)
{
	return {};
}

template <typename K, typename V>
HashMapIter<const HashMap<K, V>> begin(const HashMap<K, V> &m)
{
	return HashMapIter<const HashMap<K, V>>(m);
}

template <typename K, typename V>
HashMapIter<const HashMap<K, V>> end(const HashMap<K, V>&)
{
	return {};
}
