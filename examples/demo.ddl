/*
	demonstration database
*/

database demo {

	define NAME_LEN		30

	data file "company.dat"  contains company;
	key  file "company.ix1"  contains company.id;
	key  file "company.ix2"  contains company.references;
	data file "product.dat"  contains product;
	key  file "product.ix1"  contains product.name;

	data file "fancy.dat"    contains fancy;
	key  file "fancy.ix1"    contains fancy.fancy_key1;
	key  file "fancy.ix2"    contains fancy.name;
	key  file "fancy.ix3"    contains fancy.fancy_key2;

	record company {
		ulong	id;
		char	name[NAME_LEN+1];
		
		primary key id;
	}

	record product {
		ulong	company_id;
		ushort	descr_len;
		char	name[NAME_LEN+1];
		char	description[2000] variable by descr_len;

		primary key name;
		foreign key company_id references company
			on update restrict
			on delete restrict;
	}

	
	// This record just shows some of the things that are possible in
	// a ddl-file.

	record fancy {
		char		name[20];
		ulong		number;

		char		key2_is_null;
		
		ushort		a_count;
		struct {
			char	a[2][20];
			uchar	type;
			
			union controlled by type {
				char	s[80];			// If type is 0
				ulong	a;				// If type is 1
				struct {
					int		a;
					long	b;
				} c;					// If type is 0
			} u[2];

		} a[10] variable by a_count;
	
		primary key fancy_key1 { name, number desc };
		alternate key name null by name;
		alternate key fancy_key2 { number desc, name } null by key2_is_null;
	}
}

/* end-of-file */

