package util;

import org.docx4j.Docx4J;
import org.docx4j.convert.in.xhtml.XHTMLImporterImpl;
import org.docx4j.fonts.IdentityPlusMapper;
import org.docx4j.fonts.Mapper;
import org.docx4j.fonts.PhysicalFont;
import org.docx4j.fonts.PhysicalFonts;
import org.docx4j.jaxb.Context;
import org.docx4j.model.structure.PageSizePaper;
import org.docx4j.openpackaging.packages.WordprocessingMLPackage;
import org.docx4j.wml.RFonts;
import org.jsoup.Jsoup;
import org.jsoup.nodes.Document;
import org.jsoup.nodes.Element;
import org.jsoup.nodes.Entities;
import org.jsoup.select.Elements;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.File;
import java.io.OutputStream;
import java.net.URL;

import javax.swing.JFileChooser;
import javax.swing.JLabel;

/**
 * 将HTML文本转换成docx或pdf。
 * 利用docx4j开源库实现
 */
public class HtmlConverter {

    private final Logger logger = LoggerFactory.getLogger(this.getClass());

    /**
     * 将页面保存为 docx
     *
     * @param html
     * @return
     * @throws Exception
     */
    public boolean saveHtmlToDocx(String html) throws Exception {
        return saveDocx(html2word(html));
    }

    /**
     * 将页面保存为 pdf
     *
     * @param html
     * @return
     * @throws Exception
     */
    public boolean saveHtmlToPdf(String html) throws Exception {
        return savePdf(html2word(html));
    }

    /**
     * 将页面转为 {@link org.docx4j.openpackaging.packages.WordprocessingMLPackage}
     *
     * @param html
     * @return
     * @throws Exception
     */
    public WordprocessingMLPackage html2word(String html) throws Exception {
        return xhtml2word(html2xhtml(html));
    }

    /**
     * 将 {@link org.docx4j.openpackaging.packages.WordprocessingMLPackage} 存为 docx
     *
     * @param wordMLPackage
     * @return
     * @throws Exception
     */
    public boolean saveDocx(WordprocessingMLPackage wordMLPackage) throws Exception {

    	File file = getFile("docx");
    	if(file == null) return false;
    	
        wordMLPackage.save(file); //保存到 docx 文件

        if (logger.isDebugEnabled()) {
            logger.debug("Save to [.docx]: {}", file.getAbsolutePath());
        }
        return true;
    }

    /**
     * 将 {@link org.docx4j.openpackaging.packages.WordprocessingMLPackage} 存为 pdf
     *
     * @param wordMLPackage
     * @return
     * @throws Exception
     */
    public boolean savePdf(WordprocessingMLPackage wordMLPackage) throws Exception {

        File file = getFile("pdf");
        if(file == null) return false;

        OutputStream os = new java.io.FileOutputStream(file);

        Docx4J.toPDF(wordMLPackage, os);

        os.flush();
        os.close();

        if (logger.isDebugEnabled()) {
            logger.debug("Save to [.pdf]: {}", file.getAbsolutePath());
        }
        return true;
    }

    /**
     * 将 {@link org.jsoup.nodes.Document} 对象转为 {@link org.docx4j.openpackaging.packages.WordprocessingMLPackage}
     * xhtml to word
     *
     * @param doc
     * @return
     * @throws Exception
     */
    protected WordprocessingMLPackage xhtml2word(Document doc) throws Exception {
        WordprocessingMLPackage wordMLPackage = WordprocessingMLPackage.createPackage(PageSizePaper.valueOf("A4"), true); //A4纸，//横版:true
//        configSimSunFont(wordMLPackage); //配置中文字体
        XHTMLImporterImpl xhtmlImporter = new XHTMLImporterImpl(wordMLPackage);

        wordMLPackage.getMainDocumentPart().getContent().addAll( //导入 xhtml
                xhtmlImporter.convert(doc.html(), doc.baseUri()));


        return wordMLPackage;
    }

    /**
     * 将页面转为{@link org.jsoup.nodes.Document}对象，xhtml 格式
     *
     * @param html
     * @return
     * @throws Exception
     */
    protected Document html2xhtml(String html) throws Exception {
        Document doc = Jsoup.parse(html);
        
        if (logger.isDebugEnabled()) {
            logger.debug("baseUri: {}", doc.baseUri());
        }

        for (Element script : doc.getElementsByTag("script")) { //除去所有 script
            script.remove();
        }

        for (Element a : doc.getElementsByTag("a")) { //除去 a 的 onclick，href 属性
            a.removeAttr("onclick");
            a.removeAttr("href");
        }

        Elements links = doc.getElementsByTag("link"); //将link中的地址替换为绝对地址
        for (Element element : links) {
            String href = element.absUrl("href");

            if (logger.isDebugEnabled()) {
                logger.debug("href: {} -> {}", element.attr("href"), href);
            }

            element.attr("href", href);
        }

        doc.outputSettings()
                .syntax(Document.OutputSettings.Syntax.xml)
                .escapeMode(Entities.EscapeMode.xhtml);  //转为 xhtml 格式

        if (logger.isDebugEnabled()) {
            String[] split = doc.html().split("\n");
            for (int c = 0; c < split.length; c++) {
                logger.debug("line {}:\t{}", c + 1, split[c]);
            }
        }
        return doc;
    }

    /**
     * 为 {@link org.docx4j.openpackaging.packages.WordprocessingMLPackage} 配置中文字体
     *
     * @param wordMLPackage
     * @throws Exception
     */
    protected void configSimSunFont(WordprocessingMLPackage wordMLPackage) throws Exception {
        Mapper fontMapper = new IdentityPlusMapper();
        wordMLPackage.setFontMapper(fontMapper);

        String fontFamily = "SimSun";

        URL simsunUrl = this.getClass().getResource("/org/noahx/html2docx/simsun.ttc"); //加载字体文件（解决linux环境下无中文字体问题）
        PhysicalFonts.addPhysicalFont(simsunUrl);
        PhysicalFont simsunFont = PhysicalFonts.get(fontFamily);
        fontMapper.put(fontFamily, simsunFont);

        RFonts rfonts = Context.getWmlObjectFactory().createRFonts(); //设置文件默认字体
        rfonts.setAsciiTheme(null);
        rfonts.setAscii(fontFamily);
        wordMLPackage.getMainDocumentPart().getPropertyResolver()
                .getDocumentDefaultRPr().setRFonts(rfonts);
    }

    /**
     * 生成文件名字
     *
     * @return
     */
    protected File getFile(String postFix) {
    	JFileChooser jf = new JFileChooser();
		jf.setSelectedFile(new File("./untitled." + postFix));
		int option = jf.showSaveDialog(new JLabel());
		if(option == JFileChooser.CANCEL_OPTION) return null;
		
		File file = jf.getSelectedFile();
		
		return file;
    }
}
