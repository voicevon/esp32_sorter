import os
import sys

# Try to use pdfplumber for better layout, or pypdf as fallback
try:
    import pdfplumber
    USE_PLUMBER = True
except ImportError:
    from pypdf import PdfReader
    USE_PLUMBER = False

import os
import glob

# Try to use pdfplumber for better layout
try:
    import pdfplumber
    USE_PLUMBER = True
except ImportError:
    from pypdf import PdfReader
    USE_PLUMBER = False

def convert_to_md(pdf_path):
    md_path = pdf_path.replace(".pdf", ".md")
    print(f"Converting {pdf_path} -> {md_path}...")
    text = ""
    try:
        if USE_PLUMBER:
            with pdfplumber.open(pdf_path) as pdf:
                for page in pdf.pages:
                    text += page.extract_text() or ""
                    text += "\n\n---\n\n"
        else:
            from pypdf import PdfReader
            reader = PdfReader(pdf_path)
            for page in reader.pages:
                text += page.extract_text() or ""
                text += "\n\n---\n\n"
        
        with open(md_path, "w", encoding="utf-8") as f:
            f.write(text)
        print(f"Done.")
    except Exception as e:
        print(f"Error: {e}")

if __name__ == "__main__":
    docs_dir = r"d:\Software\antigravity\esp32_sorter\doc\Servo driver"
    pdfs = glob.glob(os.path.join(docs_dir, "*.pdf"))
    for pdf in pdfs:
        convert_to_md(pdf)
