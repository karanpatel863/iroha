node('master') {
  def scmVars = checkout scm
  def test = load '.jenkinsci/test-class.groovy'
  def t = new test.Worker()
  t.label = 'LABEL'
  def a = t.label
  sh "echo ${a}"
}
