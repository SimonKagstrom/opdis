/*!
 * \file model.h
 * \brief Data model for libopdis.
 * \details This defines the data model for libopdis.
 * \author TG Community Developers <community@thoughtgang.org>
 * \note Copyright (c) 2010 ThoughtGang.
 * Released under the GNU Lesser Public License (LGPL), version 3.
 * See http://www.gnu.org/licenses/gpl.txt for details.
 */

#ifndef OPDIS_MODEL_H
#define OPDIS_MODEL_H

#include <opdis/metadata.h>
#include <opdis/types.h>

#ifdef WIN32
        #define LIBCALL _stdcall
#else
        #define LIBCALL
#endif

/*! \enum opdis_insn_status_t
 *  \ingroup model
 *  \brief
 *  \sa opdis_insn_t
 */
enum opdis_insn_decode_t {
	opdis_decode_invalid = 0,	/*!< invalid instruction */
	opdis_decode_basic = 1,		/*!< ascii, offset, vma, size, bytes */
	opdis_decode_mnem = 2,		/*!< mnemonic, prefixes parsed */
	opdis_decode_ops = 4,		/*!< operand list and dest/src/tgt */
	opdis_decode_mnem_flags = 8,	/*!< insn category, flags decoded */
	opdis_decode_op_flags = 16	/*!< operand category, flags decoded */
};

/* ---------------------------------------------------------------------- */
/* OPERAND */

/*! 
 * \def OPDIS_REG_NAME_SZ
 * Max size of an operand register name.
 */
#define OPDIS_REG_NAME_SZ 16

/*!
 * \struct opdis_reg_t 
 * \ingroup model
 * \brief CPU Register operand
 * \details A register operand, e.g. EAX in the x86 architecture.
 * \sa opdis_op_t
 */
typedef struct {
	char ascii[OPDIS_REG_NAME_SZ];	/*!< Name of register */
	enum opdis_reg_flag_t flags;	/*!< Type of register */
	unsigned char id;		/*!< Register id # */
	unsigned char size;		/*!< Size of register in bytes */
} opdis_reg_t;

/*!
 * \struct opdis_abs_addr_t 
 * \ingroup model
 * \brief An absolute address operand
 * \details A segment:offset address, eg CS:0x401000 in the x86 architecture.
 * \sa opdis_op_t
 */
typedef struct {
	opdis_reg_t segment;
	uint64_t offset;
} opdis_abs_addr_t;

/*!
 * \enum opdis_addr_expr_elem_t 
 * \ingroup model
 * \brief Elements present in an address expression.
 * \sa opdis_addr_expr_t
 * \note Scale factor is always present; it defaults to 1.
 */
enum opdis_addr_expr_elem_t {
	opdis_addr_expr_base = 1,	/*!< Base register */
	opdis_addr_expr_index = 2,	/*!< Index register */
	opdis_addr_expr_disp = 4,	/*!< Displacement */
	opdis_addr_expr_disp_u = 8,	/*!< Unsigned disp */
	opdis_addr_expr_disp_s = 16,	/*!< Signed disp */
	opdis_addr_expr_disp_abs = 32	/*!< Absolute addr disp */
};

/*!
 * \enum opdis_addr_expr_shift_t 
 * \ingroup model
 * \brief Type of shift operation used in address expression.
 * \sa opdis_addr_expr_t
 * \note These only apply to ARM; x86 is always ASL.
 */
enum opdis_addr_expr_shift_t {
	opdis_addr_expr_lsl,		/*!< Logical shift left */
	opdis_addr_expr_lsr,		/*!< Logical shift right */
	opdis_addr_expr_asl,		/*!< Arithmetic shift left */
	opdis_addr_expr_ror,		/*!< Rotate right */
	opdis_addr_expr_rrx		/*!< Rotate right with carry */
};

/*!
 * \struct opdis_addr_expr_t 
 * \ingroup model
 * \brief An address expression operand
 * \details An address expression or "effective address" operand. This consists
 *          of a displacement or absolute address, a base register, an index
 *          register, a scale factor, and a scale operation. All of these
 *          components are optional. The general format in x86 is
 *          \code
 *          segment:[base + index * scale + displacement]
 *          \endcode
 *          for Intel syntax and
 *          \code
 *          segment:displacement(base,index,scale)
 *          \endcode
 *          for AT&T syntax.<p>
 *          In general, the scale operation is always a left shift, though in
 *          the ARM architecture additional scale operations can be specified.
 * \sa opdis_op_t
 */
typedef struct {
	enum opdis_addr_expr_elem_t elements;
	enum opdis_addr_expr_shift_t shift;
	char scale; 
	opdis_reg_t index;
	opdis_reg_t base;
	union {
		uint64_t u;
		int32_t s;
		opdis_abs_addr_t a;
	} displacement;
} opdis_addr_expr_t;

/*!
 * \struct opdis_op_t 
 * \ingroup model
 * \brief Operand object
 * \details  An instruction operand (i.e. an argument to a CPU opcode).
 * \sa opdis_insn_t
 */

typedef struct {
	char * ascii;			/*!< String representation of operand */
	enum opdis_op_cat_t category;	/*!< Type of operand, e.g. register */
	enum opdis_op_flag_t flags;	/*!< Flags for operand, e.g. signed */
	union {
		opdis_reg_t reg;	/*!< Register value */
		opdis_addr_expr_t expr;	/*!< Address expression value */
		opdis_abs_addr_t abs;	/*!< Absolute address value */
		union {
			opdis_vma_t vma;/*!< Virtual memory address */
			uint64_t u;	/*!< Unsigned immediate value */
			int64_t s;	/*!< Signed immediate value */
		} immediate;		/*!< Immediate value */
	} value;			/*!< Value of operand */
	unsigned char data_size;	/*!< Size of operand datatype */

	/* fixed-size operand fields */
	unsigned char fixed_size;	/*!< Is op of a fixed size? 0 or 1 */
	unsigned char ascii_sz;		/*!< Size of fixed ascii field */
} opdis_op_t;

/* ---------------------------------------------------------------------- */
/* INSTRUCTION */

/*!
 * \struct opdis_insn_t 
 * \ingroup model
 * \brief Instruction object
 * \details A disassembled instruction. Depending on the decoder, some or
 *          all of the fields will be set. The \e status field specifies 
 *	    what information is present.
 * \note The \e ascii field always contains the raw libopcodes output
 *       for the instruction.
 * \note The \e offset field is always set to the offset of the instruction
 *       in the buffer. By default, the \e vma field will be set to the
 *       value in \e offset. The \ref OPDIS_HANDLER callback can set 
 *       \e vma to the load address of the instruction.
 * \note For instructions allocated by opdis_insn_alloc, \e num_operands
 *       and \e alloc_operands will be the same. For instructions allocated by
 *       opdis_insn_alloc_fixed, \e num_operands will contain the number of
 *       operands in the instruction, and \e alloc_operands will contain the
 *       number of fixed_size operands that have been allocated.
 * \sa opdis_op_t
 */
typedef struct {
	enum opdis_insn_decode_t status;/*!< Result of decoding */
	char * ascii;			/*!< String representation of insn */

	opdis_off_t offset;		/*!< Offset of instruction in buffer */
	opdis_vma_t vma;		/*!< Virtual memory address of insn */

	opdis_off_t size;		/*!< Size (# bytes) of insn */
	opdis_byte_t * bytes;		/*!< Array of insn bytes */

	/* instruction  */
	opdis_off_t num_prefixes;	/*!< Number of prefixes in insn */
	char * prefixes;		/*!< Space-delimited prefix strings */

	char * mnemonic;		/*!< ASCII mnemonic for insn opcode */
	enum opdis_insn_cat_t category;	/*!< Type of insn opcode */
	enum opdis_insn_subset_t isa;	/*!< Subset of ISA for insn opcode */
	union {
		enum opdis_cflow_flag_t cflow;	/*!< Control flow insn flags */
		enum opdis_stack_flag_t stack;	/*!< Stack insn flags */
		enum opdis_io_flag_t io;	/*!< IO Port insn flags */
		enum opdis_bit_flag_t bit;	/*!< Bitwise insn flags */
	} flags;			/*!< Instruction-specific flags */
	char * comment;			/*!< Comment or hint from libopcodes */

	
	/* operands */
	opdis_off_t num_operands;	/*!< Number of operands in insn */
	opdis_off_t alloc_operands;	/*!< Number of allocated operands */
	opdis_op_t ** operands;		/*!< Array of operand objects */

	/* accessors for special operands */
	opdis_op_t * target;		/*!< Branch target */
	opdis_op_t * dest;		/*!< Destination operand */
	opdis_op_t * src;		/*!< Source operand */

	/* fixed-size instruction fields */
	unsigned char fixed_size;	/*!< Is insn of a fixed size? 0 or 1 */
	unsigned char ascii_sz;		/*!< Size of fixed ascii field */
	unsigned char mnemonic_sz;	/*!< Size of fixed mnemonic field */

} opdis_insn_t;

/* ---------------------------------------------------------------------- */
#ifdef __cplusplus
extern "C"
{
#endif


/*!
 * \fn opdis_insn_t * opdis_insn_alloc( opdis_off_t )
 * \ingroup model
 * \brief Allocate an instruction object and initialize its contents to zero.
 * \param num_operands The number of operands to allocate, or 0.
 * \return The allocated instruction.
 * \sa opdis_insn_free
 * \note The \e ascii and \e mnemonic fields are not allocated.
 * \note The operands array is allocated as an empty array of pointers; the 
 *       operands themselves are not allocated.
 */

opdis_insn_t * LIBCALL opdis_insn_alloc(  opdis_off_t num_operands );

/*!
 * \fn opdis_insn_t * opdis_insn_alloc_fixed( size_t, size_t, size_t, size_t )
 * \ingroup model
 * \brief Allocate a fixed-size instruction object for use as a buffer.
 * \details This allocates an instruction object with the specified number
 *          of operands, and with \e ascii and \e mnemonic allocated to
 *          the specified sizes. Each operand is allocated by 
 *          opdis_op_alloc_fixed.
 * \param ascii_sz
 * \param mnemonic_sz
 * \param num_operands
 * \param op_ascii_sz
 * \return The allocated instruction.
 * \sa opdis_insn_alloc
 * \sa opdis_insn_free
 * \note The instruction object returned by this function is intended for use
 *       as a buffer. 
 */
opdis_insn_t * LIBCALL opdis_insn_alloc_fixed( size_t ascii_sz, 
				size_t mnemonic_sz, size_t num_operands,
				size_t op_ascii_sz );

/*!
 * \fn opdis_insn_t * opdis_insn_dupe( const opdis_insn_t * )
 * \ingroup model
 * \brief Duplicate an instruction object
 * \details Allocate an instruction object and initialize it with the contents
 *          of \e i. This is primarily used to create an instruction object
 *          from a fixed-size opdis_insn_t. The \e ascii, \e mnemonic, and
 *          \e operands fields are only as large as they need to be (i.e.
 *          the length of the string and the number of valid operands).
 * \param i The instruction to duplicate.
 * \return The duplicate instruction.
 * \sa opdis_insn_alloc
 */
opdis_insn_t * LIBCALL opdis_insn_dupe( const opdis_insn_t * i );

/*!
 * \fn void opdis_insn_clear( opdis_insn_t * )
 * \ingroup model
 * \brief Clear the contents of an instruction object.
 * \param i The instruction to clear.
 */
void LIBCALL opdis_insn_clear( opdis_insn_t * i );

/*!
 * \fn void opdis_insn_free( opdis_insn_t * )
 * \ingroup model
 * \brief Free an allocated instruction object.
 * \param i The instruction to free.
 * \sa opdis_insn_alloc
 * \note This frees \e ascii, \e mnemonic, and all allocated operands.
 */
void LIBCALL opdis_insn_free( opdis_insn_t * i );

/*!
 * \fn void opdis_insn_set_ascii( opdis_insn_t *, const char * )
 * \ingroup model
 * \brief Set the \e ascii field of an instruction.
 * \details This duplicates the string \e ascii and sets the \e ascii field
 *          of \e i to the new string. If the \e ascii field is non-NULL, it
 *          is freed before the assignment.
 * \param i The instruction to modify.
 * \param ascii The new value for the \e ascii field.
 * \sa opdis_insn_set_mnemonic
 */
void LIBCALL opdis_insn_set_ascii( opdis_insn_t * i, const char * ascii );

/*!
 * \fn void opdis_insn_set_mnemonic( opdis_insn_t *, const char * )
 * \ingroup model
 * \brief Set the \e mnemonic field of an instruction.
 * \details This duplicates the string \e mnemonic and sets the \e mnemonic
 *          field of \e i to the new string. If the \e mnemonic field is
 *          non-NULL, it is freed before the assignment.
 * \param i The instruction to modify.
 * \param mnemonic The new value for the \e mnemonic field.
 * \sa opdis_insn_set_ascii
 */
void LIBCALL opdis_insn_set_mnemonic( opdis_insn_t * i, const char * mnemonic );

/*!
 * a
 * a
 * \fn void opdis_insn_add_prefix( opdis_insn_t *, const char * )
 * \ingroup model
 * \brief Append a string to the \e prefix field
 * \param i The instruction to modify.
 * \param prefix The value to append to the \e prefix field.
 */
void LIBCALL opdis_insn_add_prefix( opdis_insn_t * i, const char * prefix );

/*!
 * \fn void opdis_insn_add_comment( opdis_insn_t *, const char * )
 * \ingroup model
 * \brief Append a string to the \e comment field
 * \param i The instruction to modify.
 * \param cmt The value to append to the \e comment field.
 */
void LIBCALL opdis_insn_add_comment( opdis_insn_t * i, const char * cmt );

/*!
 * \fn int opdis_insn_add_operand( opdis_insn_t *, opdis_op_t * )
 * \ingroup model
 * \brief Add an operand to an instruction.
 * \details Append an operand to the list of operands in the instruction.
 *          This does \e not duplicate the operand; it performs a realloc
 *          on the \e operands array, appends the pointer \e op to it, and
 *          increases the instruction count.
 * \param i The instruction to modify.
 * \param op The operand to append to the instruction.
 * \return 1 on success, 0 on failure.
 * \note If the number of operands in \e i is less than the number of
 *       allocated operands in \e i, no realloc is performed.
 */
int LIBCALL opdis_insn_add_operand( opdis_insn_t * i, opdis_op_t * op );

/*!
 * \fn opdis_op_t * opdis_insn_next_avail_op( opdis_insn_t * )
 * \ingroup model
 * \brief Return next available allocated operand.
 * \param i The instruction to containing the operand.
 * \return The next unused operand, or NULL if all allocated operands are used.
 */
opdis_op_t * LIBCALL opdis_insn_next_avail_op( opdis_insn_t * i );

/*!
 * \fn int opdis_insn_is_branch( opdis_insn_t * )
 * \ingroup model
 * \brief Determine if the instruction has a branch target operand
 * \details Returns true (1) if the instruction has a branch target
 *          operand. The branch target operand is accessible via
 *          \e i->target.
 * \param i The instruction to examine.
 * \return 1 if the instruction has a branch target, 0 otherwise.
 * \note This will only work if of the decoder supports it. Check
 *       that status contains both \e opdis_decode_mnem_flags and
 *       opdis_decode_ops before relying on the return value.
 */
int LIBCALL opdis_insn_is_branch( opdis_insn_t * insn );

/*!
 * \fn int opdis_insn_fallthrough( opdis_insn_t * )
 * \ingroup model
 * \brief Determine if the execution falls through to the next instruction.
 * \details Returns true (1) if execution falls through to the subsequent
 *          instruction in memory. This is true for all instructions except
 *          unconditional jumps (JMP) and procedure returns (RET).
 * \param i The instruction to examine.
 * \return 1 if execution continues, 0 otherwise.
 * \note This will only work if of the decoder supports it. Check
 *       that status contains \e opdis_decode_mnem_flags 
 *       before relying on the return value.
 */
int LIBCALL opdis_insn_fallthrough( opdis_insn_t * insn );

/*!
 * \fn int opdis_insn_isa_str( const opdis_insn_t *, char *, int )
 * \ingroup model
 * \brief Generate a string representation of instruction isa field.
 * \param i The instruction.
 * \param buf The buffer to append the string to.
 * \param buf_len The length of the buffer.
 * \sa opdis_insn_cat_str
 * \sa opdis_insn_flags_str
 * \note If \e buf is not an empty string, it will be appended (not replaced).
 */
int LIBCALL opdis_insn_isa_str( const opdis_insn_t * i, char * buf, 
				int buf_len );

/*!
 * \fn int opdis_insn_cat_str( const opdis_insn_t *, char *, int )
 * \ingroup model
 * \brief Generate a string representation of instruction category field.
 * \param i The instruction.
 * \param buf The buffer to append the string to.
 * \param buf_len The length of the buffer.
 * \note '/' should not be used as a delimiter, as some flags (e.g. load/store,
 *       i/o) use it in their string representation.
 * \sa opdis_insn_isa_str
 * \sa opdis_insn_flags_str
 * \note If \e buf is not an empty string, it will be appended (not replaced).
 */
int LIBCALL opdis_insn_cat_str( const opdis_insn_t * i, char * buf, 
				int buf_len );

/*!
 * \fn int opdis_insn_flags_str(const opdis_insn_t *, char *, int, const char *)
 * \ingroup model
 * \brief Generate a string representation of instruction flags field.
 * \param i The instruction.
 * \param buf The buffer to append the string to.
 * \param buf_len The length of the buffer.
 * \param delim The delimiter to use between flags.
 * \sa opdis_insn_cat_str
 * \sa opdis_insn_isa_str
 * \note If \e buf is not an empty string, it will be appended (not replaced).
 */
int LIBCALL opdis_insn_flags_str( const opdis_insn_t * i, char * buf, 
				  int buf_len, const char * delim );

/*!
 * \fn opdis_op_t * opdis_op_alloc()
 * \ingroup model
 * \brief Allocate an operand object.
 * \return The allocated operand.
 * \sa opdis_op_free
 */
opdis_op_t * LIBCALL opdis_op_alloc( void );

/*!
 * \fn opdis_op_t * opdis_op_alloc_fixed( size_t )
 * \ingroup model
 * \brief Allocate a fixed-size operand object for use as a buffer.
 * \details This allocates an operand object with \e ascii allocated to
 *          the specified size. 
 * \param ascii_sz
 * \return The allocated operand.
 * \sa opdis_op_alloc
 * \sa opdis_op_free
 */
opdis_op_t * LIBCALL opdis_op_alloc_fixed( size_t ascii_sz );

/*! 
 * \fn opdis_op_t * opdis_op_dupe( opdis_op_t * )
 * \ingroup model
 * \brief Duplicate an operand object
 * \details Allocate an operand object and initialize it with the contents
 *          of \e op. This is primarily used to create an operand object
 *          from a fixed-size opdis_op_t.
 * \param op The operand to duplicate.
 * \return The duplicate operand.
 * \sa opdis_op_alloc
 * \sa opdis_insn_dupe
 */
opdis_op_t * LIBCALL opdis_op_dupe(  opdis_op_t * op );

/*!
 * \fn void opdis_op_clear( opdis_op_t * )
 * \ingroup model
 * \brief Clear the contents of an operand object.
 * \param o The operand to clear.
 */
void LIBCALL opdis_op_clear( opdis_op_t * o );

/*!
 * \fn void opdis_op_free( opdis_op_t * )
 * \ingroup model
 * \brief Free an allocated operand object.
 * \param op The operand to free.
 * \sa opdis_op_alloc
 */
void LIBCALL opdis_op_free( opdis_op_t * op );

/*!
 * \fn void opdis_op_set_ascii( opdis_op_t *, const char * )
 * \ingroup model
 * \brief Set the \e ascii field of an operand.
 * \details This duplicates the string \e ascii and sets the \e ascii field
 *          of \e op to the new string. If the \e ascii field is non-NULL, it
 *          is freed before the assignment.
 * \param op The operand to modify
 * \param ascii The new value for the \e ascii field.
 */
void LIBCALL opdis_op_set_ascii( opdis_op_t * op, const char * ascii );

/*!
 * \fn int opdis_op_cat_str( const opdis_op_t *, char *, int )
 * \ingroup model
 * \brief Generate a string representation of operand category field.
 * \param op The operand.
 * \param buf The buffer to append the string to.
 * \param buf_len The length of the buffer.
 * \note If \e buf is not an empty string, it will be appended (not replaced).
 * \sa opdis_op_flags_str
 * \sa opdis_reg_flags_str
 */
int LIBCALL opdis_op_cat_str( const opdis_op_t * op, char * buf, int buf_len );

/*!
 * \fn int opdis_op_flags_str( const opdis_op_t *, char *, int, const char * )
 * \ingroup model
 * \brief Generate a string representation of operand flags field.
 * \param op The operand.
 * \param buf The buffer to append the string to.
 * \param buf_len The length of the buffer.
 * \param delim The delimiter to use between flags.
 * \note If \e buf is not an empty string, it will be appended (not replaced).
 * \sa opdis_op_cat_str
 * \sa opdis_reg_flags_str
 */
int LIBCALL opdis_op_flags_str( const opdis_op_t * op, char * buf, int buf_len,
				const char * delim );

/*!
 * \fn int opdis_reg_flags_str( const opdis_reg_t *, char *, int, const char * )
 * \ingroup model
 * \brief Generate a string representation of register flags field.
 * \param reg The register.
 * \param buf The buffer to append the string to.
 * \param buf_len The length of the buffer.
 * \param delim The delimiter to use between flags.
 * \note If \e buf is not an empty string, it will be appended (not replaced).
 * \sa opdis_op_cat_str
 * \sa opdis_op_flags_str
 */
int LIBCALL opdis_reg_flags_str( const opdis_reg_t * reg, char * buf, 
				 int buf_len, const char * delim );
/*!
 * \fn int opdis_addr_expr_shift_str( const opdis_addr_expr_t *, char *, int )
 * \ingroup model
 * \brief Generate a string representation of addr expression shift field.
 * \param exp The address expression.
 * \param buf The buffer to append the string to.
 * \param buf_len The length of the buffer.
 * \note If \e buf is not an empty string, it will be appended (not replaced).
 * \sa opdis_op_cat_str
 * \sa opdis_op_flags_str
 */
int LIBCALL opdis_addr_expr_shift_str( const opdis_addr_expr_t * exp, 
					char * buf, int buf_len );

#ifdef __cplusplus
}
#endif

#endif
