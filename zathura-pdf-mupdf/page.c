/* See LICENSE file for license and copyright information */

#include "plugin.h"
#include "internal.h"

zathura_error_t
pdf_page_init(zathura_page_t* page)
{
  if (page == NULL) {
    return ZATHURA_ERROR_INVALID_ARGUMENTS;
  }

  zathura_error_t error = ZATHURA_ERROR_OK;

  zathura_document_t* document;
  if ((error = zathura_page_get_document(page, &document)) != ZATHURA_ERROR_OK) {
    return error;
  }

  mupdf_document_t* mupdf_document;
  if ((error = zathura_document_get_user_data(document, (void**) &mupdf_document)) != ZATHURA_ERROR_OK
      || mupdf_document == NULL) {
    goto error_out;
  }

  /* init data */
  unsigned int index;
  if ((error = zathura_page_get_index(page, &index)) != ZATHURA_ERROR_OK) {
    goto error_out;
  }

  mupdf_page_t* mupdf_page = calloc(1, sizeof(mupdf_page_t));
  if (mupdf_page == NULL) {
    error = ZATHURA_ERROR_OUT_OF_MEMORY;
    goto error_out;
  }

  if ((error = zathura_page_set_user_data(page, mupdf_page)) != ZATHURA_ERROR_OK) {
    free(mupdf_page);
    goto error_free;
  }

  mupdf_page->ctx = mupdf_document->ctx;
  if (mupdf_page->ctx == NULL) {
    goto error_free;
  }

  fz_try (mupdf_page->ctx) {
    mupdf_page->page = fz_load_page(mupdf_document->ctx, mupdf_document->document, index);
  } fz_catch (mupdf_page->ctx) {
    goto error_free;
  }

  mupdf_page->bbox = fz_bound_page(mupdf_document->ctx, (fz_page*) mupdf_page->page);

  /* calculate dimensions */
  double width  = mupdf_page->bbox.x1 - mupdf_page->bbox.x0;
  double height = mupdf_page->bbox.y1 - mupdf_page->bbox.y0;

  if ((error = zathura_page_set_width(page, width)) != ZATHURA_ERROR_OK) {
    free(mupdf_page);
    goto error_free;
  }

  if ((error = zathura_page_set_height(page, height)) != ZATHURA_ERROR_OK) {
    free(mupdf_page);
    goto error_free;
  }

  /* setup text */
  mupdf_page->extracted_text = false;

  fz_stext_options stext_options = { FZ_STEXT_PRESERVE_IMAGES };
  mupdf_page->text = fz_new_stext_page_from_page(mupdf_page->ctx, mupdf_page->page, &stext_options);
  if (mupdf_page->text == NULL) {
    goto error_free;
  }

error_out:

  return error;

error_free:

  pdf_page_clear(page);

  return error;
}

zathura_error_t
pdf_page_clear(zathura_page_t* page)
{
  if (page == NULL) {
    return ZATHURA_ERROR_INVALID_ARGUMENTS;
  }

  zathura_error_t error = ZATHURA_ERROR_OK;

  mupdf_page_t* mupdf_page;
  if ((error = zathura_page_get_user_data(page, (void**) &mupdf_page)) != ZATHURA_ERROR_OK) {
    return error;
  }

  if (mupdf_page->text != NULL) {
    fz_drop_stext_page(mupdf_page->ctx, mupdf_page->text);
  }

  if (mupdf_page->page != NULL) {
    fz_drop_page(mupdf_page->ctx, mupdf_page->page);
  }

  free(mupdf_page);

  return ZATHURA_ERROR_OK;
}
